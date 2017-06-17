
#ifndef __JS_SPECIFICS_H__
#define __JS_SPECIFICS_H__

#include "ScriptingCore.h"
#include "platform/CASAXParser.h"

class JSScheduleWrapper;

// JSScheduleWrapper* --> Array* since one js function may correspond to many targets.
// To debug this, you could refer to JSScheduleWrapper::dump function.
// It will prove that i'm right. :)
typedef struct jsScheduleFunc_proxy {
    JS::Heap<JSObject*> jsfuncObj;
    CrossApp::CAVector<CAObject*>*  targets;
    UT_hash_handle hh;
} schedFunc_proxy_t;

typedef struct jsScheduleTarget_proxy {
    JS::Heap<JSObject*> jsTargetObj;
    CrossApp::CAVector<CAObject*>*  targets;
    UT_hash_handle hh;
} schedTarget_proxy_t;


typedef struct jsCallFuncTarget_proxy {
    void * ptr;
    CrossApp::CAVector<CAObject*> *obj;
    UT_hash_handle hh;
} callfuncTarget_proxy_t;

extern schedFunc_proxy_t *_schedFunc_target_ht;
extern schedTarget_proxy_t *_schedObj_target_ht;

extern callfuncTarget_proxy_t *_callfuncTarget_native_ht;

/**
 * You don't need to manage the returned pointer. They live for the whole life of
 * the app.
 */
template <class T>
inline js_type_class_t *js_get_type_from_native(T* native_obj) {
    bool found = false;
    std::string typeName = typeid(*native_obj).name();
    auto typeProxyIter = _js_global_type_map.find(typeName);
    if (typeProxyIter == _js_global_type_map.end())
    {
        typeName = typeid(T).name();
        typeProxyIter = _js_global_type_map.find(typeName);
        if (typeProxyIter != _js_global_type_map.end())
        {
            found = true;
        }
    }
    else
    {
        found = true;
    }
    return found ? typeProxyIter->second : nullptr;
}

/**
 * The returned pointer should be deleted using jsb_remove_proxy. Most of the
 * time you do that in the C++ destructor.
 */
template<class T>
inline js_proxy_t *js_get_or_create_proxy(JSContext *cx, T *native_obj) {
    js_proxy_t *proxy;
    HASH_FIND_PTR(_native_js_global_ht, &native_obj, proxy);
    if (!proxy) {
        js_type_class_t *typeProxy = js_get_type_from_native<T>(native_obj);
        // Return NULL if can't find its type rather than making an assert.
//        assert(typeProxy);
        if (!typeProxy) {
//            CCLOGINFO("Could not find the type of native object.");
            return NULL;
        }
        
        JSB_AUTOCOMPARTMENT_WITH_GLOBAL_OBJCET

        JS::RootedObject proto(cx, const_cast<JSObject*>(typeProxy->proto.get()));
        JS::RootedObject parent(cx, const_cast<JSObject*>(typeProxy->parentProto.get()));
        JS::RootedObject js_obj(cx, JS_NewObject(cx, typeProxy->jsclass, proto, parent));
        proxy = jsb_new_proxy(native_obj, js_obj);
#ifdef DEBUG
//        AddNamedObjectRoot(cx, &proxy->obj, typeid(*native_obj).name());
#else
        AddObjectRoot(cx, &proxy->obj);
#endif
        return proxy;
    } else {
        return proxy;
    }
    return NULL;
}

JS::Value anonEvaluate(JSContext *cx, JS::HandleObject thisObj, const char* string);
void register_crossapp_js_core(JSContext* cx, JS::HandleObject obj);


void js_add_object_reference(JS::HandleValue owner, JS::HandleValue target);
void js_remove_object_reference(JS::HandleValue owner, JS::HandleValue target);
void js_add_object_root(JS::HandleValue target);
void js_remove_object_root(JS::HandleValue target);

JS::Value anonEvaluate(JSContext *cx, JS::HandleObject thisObj, const char* string);
void register_cocos2dx_js_core(JSContext* cx, JS::HandleObject obj);

class JSCallbackWrapper: public CrossApp::CAObject {
public:
    JSCallbackWrapper();
    virtual ~JSCallbackWrapper();
    void setJSCallbackFunc(JS::HandleValue callback);
    void setJSCallbackThis(JS::HandleValue thisObj);
    void setJSExtraData(JS::HandleValue data);
    
    const jsval getJSCallbackFunc() const;
    const jsval getJSCallbackThis() const;
    const jsval getJSExtraData() const;
protected:
    mozilla::Maybe<JS::PersistentRootedValue> _jsCallback;
    mozilla::Maybe<JS::PersistentRootedValue> _jsThisObj;
    mozilla::Maybe<JS::PersistentRootedValue> _extraData;
};

class JSTouchDelegate: public CrossApp::CAResponder
{
public:
    JSTouchDelegate();
    virtual ~JSTouchDelegate();
    
    // Set the touch delegate to map by using the key (pJSObj).
    static void setDelegateForJSObject(JSObject* pJSObj, JSTouchDelegate* pDelegate);
    // Get the touch delegate by the key (pJSObj).
    static JSTouchDelegate* getDelegateForJSObject(JSObject* pJSObj);
    // Remove the delegate by the key (pJSObj).
    static void removeDelegateForJSObject(JSObject* pJSObj);
    
    void setJSObject(JS::HandleObject obj);

    virtual bool onTouchBegan(CrossApp::CATouch *touch, CrossApp::CAEvent *event);
    virtual void onTouchMoved(CrossApp::CATouch *touch, CrossApp::CAEvent *event);
    virtual void onTouchEnded(CrossApp::CATouch *touch, CrossApp::CAEvent *event);
    virtual void onTouchCancelled(CrossApp::CATouch *touch, CrossApp::CAEvent *event);
    
    //mouse
    virtual void mouseMoved(CATouch* pTouch, CAEvent* pEvent);
    virtual void mouseMovedOutSide(CATouch* pTouch, CAEvent* pEvent);
    virtual void mouseScrollWheel(CATouch* pTouch, float off_x, float off_y, CAEvent* pEvent);


private:
    mozilla::Maybe<JS::PersistentRootedObject> _obj;
    typedef std::unordered_map<JSObject*, JSTouchDelegate*> TouchDelegateMap;
    typedef std::pair<JSObject*, JSTouchDelegate*> TouchDelegatePair;
    static TouchDelegateMap sTouchDelegateMap;
};

class JSTouchView : public CrossApp::CAView
{
public:
    JSTouchView();
    ~JSTouchView();
    
    static void setTouchViewForJSObject(JSObject* pJSObj, JSTouchView* pTouchView);
    
    static JSTouchView* getTouchViewForJSObject(JSObject* pJSObj);
    
    static void removeTouchViewForJSObject(JSObject* pJSObj);
    
    void setJSObject(JS::HandleObject obj);
    
    DRect getBounds() const;
    
    
    virtual bool ccTouchBegan(CrossApp::CATouch *touch, CrossApp::CAEvent *event);
    virtual void ccTouchMoved(CrossApp::CATouch *touch, CrossApp::CAEvent *event);
    virtual void ccTouchEnded(CrossApp::CATouch *touch, CrossApp::CAEvent *event);
    virtual void ccTouchCancelled(CrossApp::CATouch *touch, CrossApp::CAEvent *event);
    
    //mouse
//    virtual void mouseMoved(CATouch* pTouch, CAEvent* pEvent);
//    virtual void mouseMovedOutSide(CATouch* pTouch, CAEvent* pEvent);
//    virtual void mouseScrollWheel(CATouch* pTouch, float off_x, float off_y, CAEvent* pEvent);
    
private:
    mozilla::Maybe<JS::PersistentRootedObject> _obj;
    typedef std::unordered_map<JSObject*, JSTouchView*> TouchViewMap;
    typedef std::pair<JSObject*, JSTouchView*> TouchViewPair;
    static TouchViewMap sTouchViewMap;
    
};




class JSTouchViewController : public CrossApp::CAView
{
public:
    JSTouchViewController();
    ~JSTouchViewController();
    
    static void setTouchViewControllerForJSObject(JSObject* pJSObj, JSTouchViewController* pTouchView);
    
    static JSTouchViewController* getTouchViewControllerForJSObject(JSObject* pJSObj);
    
    static void removeTouchViewControllerForJSObject(JSObject* pJSObj);
    
    void setJSObject(JS::HandleObject obj);
    
    DRect getBounds() const;
    
    virtual bool ccTouchBegan(CrossApp::CATouch *touch, CrossApp::CAEvent *event);
    virtual void ccTouchMoved(CrossApp::CATouch *touch, CrossApp::CAEvent *event);
    virtual void ccTouchEnded(CrossApp::CATouch *touch, CrossApp::CAEvent *event);
    virtual void ccTouchCancelled(CrossApp::CATouch *touch, CrossApp::CAEvent *event);
private:
    mozilla::Maybe<JS::PersistentRootedObject> _obj;
    typedef std::unordered_map<JSObject*, JSTouchViewController*> TouchViewControllerMap;
    typedef std::pair<JSObject*, JSTouchViewController*> TouchViewControllerPair;
    static TouchViewControllerMap sTouchViewControllerMap;
};

class JSKeypadDelegate :public CrossApp::CAKeypadDelegate{
public:
    JSKeypadDelegate();
    ~JSKeypadDelegate();
    
    static void setJSKeypadDelegateForJSObject(JSObject* pJSObj, JSKeypadDelegate* pTouchView);
    
    static JSKeypadDelegate* getJSKeypadDelegateForJSObject(JSObject* pJSObj);
    
    static void removeJSKeypadDelegate(JSObject* pJSObj);
    
    void setJSObject(JS::HandleObject obj);
    
    virtual void keyBackClicked();
    
    virtual void keyMenuClicked();
private:
    mozilla::Maybe<JS::PersistentRootedObject> _obj;
    typedef std::unordered_map<JSObject*, JSKeypadDelegate*> JSKeypadDelegateMap;
    typedef std::pair<JSObject*, JSKeypadDelegate*> JSKeypadDelegatePair;
    static JSKeypadDelegateMap sJSKeypadDelegateMap;
};

bool js_crossapp_Node_onEnter(JSContext *cx, uint32_t argc, jsval *vp);
bool js_crossapp_Node_onExit(JSContext *cx, uint32_t argc, jsval *vp);
bool js_crossapp_Node_onEnterTransitionDidFinish(JSContext *cx, uint32_t argc, jsval *vp);
bool js_crossapp_Node_onExitTransitionDidStart(JSContext *cx, uint32_t argc, jsval *vp);
bool js_crossapp_Component_onEnter(JSContext *cx, uint32_t argc, jsval *vp);
bool js_crossapp_Component_onExit(JSContext *cx, uint32_t argc, jsval *vp);

bool js_crossapp_retain(JSContext *cx, uint32_t argc, jsval *vp);
bool js_crossapp_release(JSContext *cx, uint32_t argc, jsval *vp);

void get_or_create_js_obj(JSContext* cx, JS::HandleObject obj, const std::string &name, JS::MutableHandleObject jsObj);

#endif
