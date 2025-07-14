#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <juce_opengl/juce_opengl.h>

//==============================================================================

#define NUM_OGL_COMPONENTS 4
#define STATE_CHANGE_MS    3000

//==============================================================================

struct OGLComponent
    : public juce::Component
    , public juce::OpenGLRenderer {
    auto genColour() const -> juce::Colour {
        auto const h = juce::Random::getSystemRandom().nextFloat();
        return juce::Colour::fromHSV(h, 1.f, 1.f, 1.f);
    }
    void paint(juce::Graphics &g) override {
        g.setColour(genColour());
        g.drawRect(getLocalBounds(), 5.f);
        g.setColour(juce::Colours::black);
        g.setFont(juce::Font{juce::FontOptions{16.f}});
        g.drawText(
            "DIMMED?", getLocalBounds(), juce::Justification::centred
        );
    }
    void newOpenGLContextCreated() override {
    }
    void renderOpenGL() override {
        auto const t = juce::Time::getMillisecondCounter();
        if (t - colourChanged_ >= STATE_CHANGE_MS) {
            colourChanged_ = t;
            fillColour_    = genColour();
        }
        juce::OpenGLHelpers::clear(fillColour_);
    }
    void openGLContextClosing() override {
    }
    juce::uint32 colourChanged_ = juce::Time::getMillisecondCounter();
    juce::Colour fillColour_    = genColour();
};

//==============================================================================

struct Canvas
    : public juce::Component
    , public juce::OpenGLRenderer {
    Canvas() {
        setPaintingIsUnclipped(true);
        setRepaintsOnMouseActivity(true);

        oglContext_.setOpenGLVersionRequired(
            juce::OpenGLContext::OpenGLVersion::openGL3_2
        );
        oglContext_.setComponentPaintingEnabled(true);
        oglContext_.setContinuousRepainting(true);
        oglContext_.setRenderer(this);
        oglContext_.attachTo(*this);

        causeBugBtn_.setPaintingIsUnclipped(true);
        causeBugBtn_.setClickingTogglesState(true);
        causeBugBtn_.setAlpha(0.5f);
        causeBugBtn_.onClick = [&] {
            for (auto &c : oglChildren_) {
                c->setVisible(juce::Random::getSystemRandom().nextBool());
            }
            repaint();
        };
        addAndMakeVisible(causeBugBtn_);

        for (auto &c : oglChildren_) {
            c = std::make_unique<OGLComponent>();
            c->setPaintingIsUnclipped(true);
            c->setRepaintsOnMouseActivity(true);
            addAndMakeVisible(*c);
        }
    }
    ~Canvas() override {
        oglContext_.detach();
        oglContext_.setRenderer(nullptr);
    }
    void resized() override {
        auto b = getLocalBounds();
        causeBugBtn_.setBounds(b.removeFromBottom(getHeight() / 4));
        auto const cw = getWidth() / static_cast<int>(oglChildren_.size());
        for (auto &c : oglChildren_) {
            c->setBounds(b.removeFromLeft(cw));
        }
    }
    void newOpenGLContextCreated() override {
        for (auto &c : oglChildren_) {
            c->newOpenGLContextCreated();
        }
    }
    void renderOpenGL() override {
        juce::OpenGLHelpers::clear(juce::Colours::black);
        auto const scale = oglContext_.getRenderingScale();

        auto const t = juce::Time::getMillisecondCounter();
        if (t - visibilityChanged_ >= STATE_CHANGE_MS) {
            visibilityChanged_ = t;
            for (auto &c : oglChildren_) {
                c->setVisible(juce::Random::getSystemRandom().nextBool());
            }
        }

        for (auto &c : oglChildren_) {
            if (! c->isVisible()) {
                continue;
            }
            auto const clip =
                c->getBounds().withY(getHeight() - c->getBottom());
            auto const sc  = clip.toFloat() * scale;
            auto const sci = sc.toNearestIntEdges();
            juce::gl::glViewport(
                sci.getX(), sci.getY(), sci.getWidth(), sci.getHeight()
            );
            juce::gl::glEnable(juce::gl::GL_SCISSOR_TEST);
            juce::gl::glScissor(
                sci.getX(), sci.getY(), sci.getWidth(), sci.getHeight()
            );
            c->renderOpenGL();
            juce::gl::glDisable(juce::gl::GL_SCISSOR_TEST);
        }
    }
    void openGLContextClosing() override {
        for (auto &c : oglChildren_) {
            c->openGLContextClosing();
        }
    }

    juce::OpenGLContext                                           oglContext_;
    std::array<std::unique_ptr<OGLComponent>, NUM_OGL_COMPONENTS> oglChildren_;
    juce::TextButton causeBugBtn_{"Cause Bug: Text should dim!"};
    juce::uint32     visibilityChanged_ = juce::Time::getMillisecondCounter();
};

//==============================================================================

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor &p
)
    : AudioProcessorEditor(&p)
    , canvas_(std::make_unique<Canvas>()) {
    addAndMakeVisible(*canvas_);
    setSize(400, 300);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
}

//==============================================================================

void
AudioPluginAudioProcessorEditor::resized() {
    canvas_->setBounds(getLocalBounds());
}
