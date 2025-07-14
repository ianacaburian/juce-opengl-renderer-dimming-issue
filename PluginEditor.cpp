#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <juce_opengl/juce_opengl.h>

//==============================================================================

#define NUM_OGL_COMPONENTS 4
#define COLOUR_CHANGE_MS 3000

//==============================================================================

struct OGLComponent
    : public juce::Component
    , public juce::OpenGLRenderer {
    OGLComponent() {
        setRepaintsOnMouseActivity(true);
    }
    auto genColour() const -> juce::Colour {
        auto const hue    = juce::Random::getSystemRandom().nextFloat();
        return juce::Colour::fromHSV(hue, 1.f, 1.f, 1.f);
    }
    void paint(juce::Graphics &g) override {
        g.setColour(genColour());
        g.drawRect(getLocalBounds(), 10.f);
    }
    void newOpenGLContextCreated() override {
    }
    void renderOpenGL() override {
        auto const t = juce::Time::getMillisecondCounter();
        if (t - colourChanged_ >= COLOUR_CHANGE_MS) {
            colourChanged_ = t;
            fillColour_ = genColour();
        }
        juce::OpenGLHelpers::clear(fillColour_);
    }
    void openGLContextClosing() override {
    }
    juce::uint32 colourChanged_ = juce::Time::getMillisecondCounter();
    juce::Colour fillColour_ = genColour();
};

//==============================================================================

struct Renderer
    : public juce::Component
    , public juce::OpenGLRenderer {
    Renderer() {
        setRepaintsOnMouseActivity(true);

        oglContext_.setOpenGLVersionRequired(
            juce::OpenGLContext::OpenGLVersion::openGL3_2
        );
        oglContext_.setComponentPaintingEnabled(true);
        oglContext_.setContinuousRepainting(true);
        oglContext_.setRenderer(this);
        oglContext_.attachTo(*this);

        for (auto &c : oglChildren_) {
            c = std::make_unique<OGLComponent>();
            addAndMakeVisible(*c);
        }
    }
    ~Renderer() override {
        oglContext_.detach();
        oglContext_.setRenderer(nullptr);
    }
    void paint(juce::Graphics &g) override {
        g.setColour(juce::Colours::red);
        g.drawRect(getLocalBounds(), 2.f);
        auto       b  = getLocalBounds();
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
};

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor &p
)
    : AudioProcessorEditor(&p)
    , processorRef(p) {
    juce::ignoreUnused(processorRef);
    renderer = std::make_unique<Renderer>();
    addAndMakeVisible(*renderer);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(400, 300);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
}

//==============================================================================

void
AudioPluginAudioProcessorEditor::resized() {
    renderer->setBounds(getLocalBounds());
}
