/*
  ==============================================================================

    blueprint_ReactApplicationRoot.cpp
    Created: 9 Dec 2018 10:20:37am

  ==============================================================================
*/

#pragma once

#include "blueprint_ReactApplicationRoot.h"


namespace blueprint
{

    ReactApplicationRoot::ReactApplicationRoot(std::shared_ptr<EcmascriptEngine> ee)
        : viewManager(this)
        , engine(ee)
    {
        JUCE_ASSERT_MESSAGE_THREAD
        jassert(ee != nullptr);

        bindNativeRenderingHooks();

#if JUCE_DEBUG
        // Enable keyboardFocus to support CTRL-D/CMD-D debug attachment.
        setWantsKeyboardFocus(true);
#endif
    }

    ReactApplicationRoot::ReactApplicationRoot()
        : ReactApplicationRoot(std::make_shared<EcmascriptEngine>()) {}

    //==============================================================================
    juce::var ReactApplicationRoot::createViewInstance (const juce::String& viewType)
    {
        return juce::var(viewManager.createViewInstance(viewType));
    }

    juce::var ReactApplicationRoot::createTextViewInstance (const juce::String& textValue)
    {
        return juce::var(viewManager.createTextViewInstance(textValue));
    }

    juce::var ReactApplicationRoot::setViewProperty (const ViewId viewId, const juce::String& name, const juce::var& value)
    {
        viewManager.setViewProperty(viewId, name, value);
        return juce::var::undefined();
    }

    juce::var ReactApplicationRoot::setRawTextValue (const ViewId viewId, const juce::String& value)
    {
        viewManager.setRawTextValue(viewId, value);
        return juce::var::undefined();
    }

    juce::var ReactApplicationRoot::addChild (const ViewId parentId, const ViewId childId, int index)
    {
        viewManager.addChild(parentId, childId, index);
        return juce::var::undefined();
    }

    juce::var ReactApplicationRoot::removeChild (const ViewId parentId, const ViewId childId)
    {
        viewManager.removeChild(parentId, childId);
        return juce::var::undefined();
    }

    juce::var ReactApplicationRoot::getRootInstanceId()
    {
        return juce::var(getViewId());
    }

    juce::var ReactApplicationRoot::resetAfterCommit()
    {
        viewManager.performRootShadowTreeLayout();
        return juce::var::undefined();
    }

    //==============================================================================
    void ReactApplicationRoot::resized()
    {
        viewManager.performRootShadowTreeLayout();
    }

    void ReactApplicationRoot::paint(juce::Graphics& g)
    {
        if (errorText)
        {
            g.fillAll(juce::Colour(0xffe14c37));
            errorText->draw(g, getLocalBounds().toFloat().reduced(10.f));
        }
        else
        {
            View::paint(g);
        }
    }

#if JUCE_DEBUG
    bool ReactApplicationRoot::keyPressed(const juce::KeyPress& key)
    {
        const auto startDebugCommand = juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0);

        if (key == startDebugCommand)
            engine->debuggerAttach();

        return true;
    }
#endif

    //==============================================================================
    juce::var ReactApplicationRoot::evaluate(const juce::File& bundle)
    {
        JUCE_ASSERT_MESSAGE_THREAD

        try
        {
            return engine->evaluate(bundle);
        }
        catch (const EcmascriptEngine::Error& err)
        {
            handleRuntimeError(err);
            return juce::var();
        }
    }

    //==============================================================================
    void ReactApplicationRoot::registerViewType(const juce::String& typeId, ViewManager::ViewFactory f)
    {
        viewManager.registerViewType(typeId, f);
    }

    //==============================================================================
    void ReactApplicationRoot::handleRuntimeError(const EcmascriptEngine::Error& err)
    {
#if ! JUCE_DEBUG
        // In release builds, we don't catch errors and show the red screen,
        // we allow the exception to raise up to the user to be handled properly
        // for a production app.
        throw err;
#endif

        JUCE_ASSERT_MESSAGE_THREAD

        DBG("");
        DBG("==== Error in JavaScript runtime. Context: ====");
        DBG(err.context);
        DBG("");
        DBG(err.what());

        errorText = std::make_unique<juce::AttributedString>(err.stack);

#if JUCE_WINDOWS
        errorText->setFont(juce::Font("Lucida Console", 18, juce::Font::FontStyleFlags::plain));
#elif JUCE_MAC
        errorText->setFont(juce::Font("Monaco", 18, juce::Font::FontStyleFlags::plain));
#else
        errorText->setFont(18);
#endif

        // Lastly, kill the ViewManager to tear down existing views and prevent
        // further view interaction
        viewManager.clearViewTables();

        repaint();
    }

    void ReactApplicationRoot::reset()
    {
        viewManager.clearViewTables();
        engine->reset();
        errorText = nullptr;
    }

    void ReactApplicationRoot::bindNativeRenderingHooks()
    {
        const auto ns = "__BlueprintNative__";

        engine->registerNativeProperty(ns, juce::JSON::parse("{}"));

        addMethodBinding<1>(ns, "createViewInstance", &ReactApplicationRoot::createViewInstance);
        addMethodBinding<1>(ns, "createTextViewInstance", &ReactApplicationRoot::createTextViewInstance);
        addMethodBinding<3>(ns, "setViewProperty", &ReactApplicationRoot::setViewProperty);
        addMethodBinding<2>(ns, "setRawTextValue", &ReactApplicationRoot::setRawTextValue);
        addMethodBinding<3>(ns, "addChild", &ReactApplicationRoot::addChild);
        addMethodBinding<2>(ns, "removeChild", &ReactApplicationRoot::removeChild);
        addMethodBinding<0>(ns, "getRootInstanceId", &ReactApplicationRoot::getRootInstanceId);
        addMethodBinding<0>(ns, "resetAfterCommit", &ReactApplicationRoot::resetAfterCommit);
    }

}
