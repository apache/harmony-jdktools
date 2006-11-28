/*
 * Copyright 2005-2006 The Apache Software Foundation or its licensors, 
 * as applicable.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/**
 * @author Pavel N. Vyssotski
 * @version $Revision: 1.12.2.1 $
 */

/**
 * @file
 * RequestManager.h
 *
 * Manages external or internal event requests activating or deactivating
 * appropriate event handlers.
 */

#ifndef _REQUEST_MANAGER_H_
#define _REQUEST_MANAGER_H_

#include <vector>

#include "AgentBase.h"
#include "AgentMonitor.h"
#include "AgentAllocator.h"
#include "AgentEventRequest.h"

namespace jdwp {

    typedef vector<AgentEventRequest*,
        AgentAllocator<AgentEventRequest*> > RequestList;

    typedef RequestList::iterator RequestListIterator;

    /**
     * The class manages events generated by the target VM and passes them to
     * <code>EventDispatcher</code>.
     */
    class RequestManager : public AgentBase {

    public:

        /**
         * A constructor.
         */
        RequestManager() throw()
            : m_requestIdCount(0)
            , m_requestMonitor(0) {}

        /**
         * A destructor.
         */
        ~RequestManager() throw() {}

        /**
         * Initializes the instance of <code>RequestManager</code>.
         *
         * @param jni - the JNI interface pointer
         *
         * @throws AgentException.
         */
        void Init(JNIEnv* jni) throw(AgentException);

        /**
         * Cleanups the instance of <code>RequestManager</code>.
         *
         * @param jni - the JNI interface pointer
         */
        void Clean(JNIEnv* jni) throw();

        /**
         * Resets the instance of <code>RequestManager</code>.
         *
         * @param jni - the JNI interface pointer
         */
        void Reset(JNIEnv* jni) throw();

        /**
         * Adds the given internal request to the list of requests of corresponding types.
         *
         * @param jni     - the JNI interface pointer
         * @param request - the <code>AgentEventRequest</code> instance pointer
         *
         * @throws AgentException.
         */
        void AddInternalRequest(JNIEnv* jni, AgentEventRequest* request)
            throw(AgentException);

        /**
         * Adds the given request to the list of requests of corresponding types 
         * and assigns the unique ID for the request.
         *
         * @param jni     - the JNI interface pointer
         * @param request - the AgentEventRequest instance pointer
         *
         * @throws AgentException.
         */
        RequestID AddRequest(JNIEnv* jni, AgentEventRequest* request)
            throw(AgentException);

        /**
         * Removes a request of the given kind with the given request ID.
         *
         * @param jni   - the JNI interface pointer
         * @param kind  - the JDWP event-request kind
         * @param id    - the request ID to delete
         *
         * @throws AgentException.
         */
        void DeleteRequest(JNIEnv* jni, jdwpEventKind kind, RequestID id)
            throw(AgentException);

        /**
         * Removes the given request from the corresponding request list.
         *
         * @param jni     - the JNI interface pointer
         * @param request - the pointer to the request to delete
         *
         * @throws AgentException.
         */
        void DeleteRequest(JNIEnv* jni, AgentEventRequest* request)
            throw(AgentException);

        /**
         * Removes all requests with the given kind from the corresponding request list.
         *
         * @param jni       - the JNI interface pointer
         * @param eventKind - the JDWP event-request kind
         *
         * @throws AgentException.
         */
        void DeleteAllRequests(JNIEnv* jni, jdwpEventKind eventKind)
            throw(AgentException);

        /**
         * Removes all requests with <code>JDWP_EVENT_BREAKPOINT</code> 
         * kind from the corresponding request list.
         *
         * @param jni - the JNI interface pointer
         *
         * @throws AgentException.
         */
        void DeleteAllBreakpoints(JNIEnv* jni) throw(AgentException);

        /**
         * Returns the name of the given JDWP event kind.
         *
         * @param kind - the JDWP event kind
         */
        const char* GetEventKindName(jdwpEventKind kind) const throw();

        /**
         * Enables an internal step request for the <code>PopFrame</code> command.
         *
         * @param jni    - the JNI interface pointer
         * @param thread - the <code>PopFrames</code> command-performed thread 
         *
         * @exception <code>AgentException</code> is thrown, if any error occurs.
         */
        void EnableInternalStepRequest(JNIEnv* jni, jthread thread) throw(AgentException);

        /**
         * Disables an internal step request after the <code>PopFrame</code> command.
         *
         * @param jni    - the JNI interface pointer
         * @param thread - the <code>PopFrames</code> command-performed thread 
         *
         * @exception <code>AgentException</code> is thrown, if any error occurs.
         */
        void DisableInternalStepRequest(JNIEnv* jni, jthread thread) throw(AgentException);

        // event callbacks

        /**
         * <code>ClassPrepare</code> event callbacks.
         *
         * @param jvmti  - the JVMTI interface pointer
         * @param jni    - the JNI interface pointer
         * @param thread - the Java thread-generating event
         * @param cls    - the prepared Java class
         */
        static void JNICALL HandleClassPrepare(jvmtiEnv* jvmti, JNIEnv* jni,
            jthread thread, jclass cls);

        /**
         * <code>ThreadEnd</code> event callbacks.
         *
         * @param jvmti  - the JVMTI interface pointer
         * @param jni    - the JNI interface pointer
         * @param thread - the Java thread-generating event
         */
        static void JNICALL HandleThreadEnd(jvmtiEnv* jvmti, JNIEnv* jni,
            jthread thread);

        /**
         * <code>ThreadStart</code> event callbacks.
         *
         * @param jvmti  - the JVMTI interface pointer
         * @param jni    - the JNI interface pointer
         * @param thread - the Java thread-generating event
         */
        static void JNICALL HandleThreadStart(jvmtiEnv* jvmti, JNIEnv* jni,
            jthread thread);

        /**
         * <code>VMInit</code> event callbacks.
         *
         * @param jvmti  - the JVMTI interface pointer
         * @param jni    - the JNI interface pointer
         * @param thread - the Java thread-generating event
         */
        static void JNICALL HandleVMInit(jvmtiEnv *jvmti, JNIEnv *jni, jthread thread);

        /**
         * <code>VMDeath</code> event callbacks.
         *
         * @param jvmti - the JVMTI interface pointer
         * @param jni   - the JNI interface pointer
         */
        static void JNICALL HandleVMDeath(jvmtiEnv *jvmti, JNIEnv *jni);

        /**
         * Breakpoint event callbacks.
         *
         * @param jvmti    - the JVMTI interface pointer
         * @param jni      - the JNI interface pointer
         * @param thread   - the Java thread-generating event
         * @param method   - the method ID where breakpoint had occurred
         * @param location - the location where breakpoint had occurred
         */
        static void JNICALL HandleBreakpoint(jvmtiEnv* jvmti, JNIEnv* jni,
            jthread thread, jmethodID method, jlocation location);

        /**
         * Exception event callbacks.
         *
         * @param jvmti          - the JVMTI interface pointer
         * @param jni            - the JNI interface pointer
         * @param thread         - the Java thread-generating event
         * @param method         - the method ID where exception had occurred
         * @param location       - the location where exception had occurred
         * @param exception      - the Java exception
         * @param catch_method   - the method ID where exception was caught
         * @param catch_location - the location where exception was caught
         */
        static void JNICALL HandleException(jvmtiEnv* jvmti, JNIEnv* jni,
            jthread thread, jmethodID method, jlocation location,
            jobject exception, jmethodID catch_method,
            jlocation catch_location);

        /**
         * <code>MethodEntry</code> event callbacks.
         *
         * @param jvmti  - the JVMTI interface pointer
         * @param jni    - the JNI interface pointer
         * @param thread - the Java thread-generating event
         * @param method - the entering method ID
         */
        static void JNICALL HandleMethodEntry(jvmtiEnv* jvmti, JNIEnv* jni,
            jthread thread, jmethodID method);

        /**
         * <code>MethodExit</code> event callbacks.
         *
         * @param jvmti                    - the JVMTI interface pointer
         * @param jni                      - the JNI interface pointer
         * @param thread                   - the Java thread-generating event
         * @param method                   - the existing method ID
         * @param was_popped_by_exception  - whether the exception-popped frame or 
         *                                   normal return occurred
         * @param return_value             - the return value
         */
        static void JNICALL HandleMethodExit(jvmtiEnv* jvmti, JNIEnv* jni,
            jthread thread, jmethodID method, jboolean was_popped_by_exception,
            jvalue return_value);

        /**
         * <code>FieldAccess</code> event callbacks.
         *
         * @param jvmti        - the JVMTI interface pointer
         * @param jni          - the JNI interface pointer
         * @param thread       - the Java thread-generating event
         * @param method       - the accessed-field method  
         * @param location     - the <code>FieldAccess</code> event location 
         * @param field_class  - the accessed-field class 
         * @param object       - the accessed-field owner object
         * @param field        - the accessed-field ID 
         */
        static void JNICALL HandleFieldAccess(jvmtiEnv* jvmti, JNIEnv* jni,
            jthread thread, jmethodID method, jlocation location,
            jclass field_class, jobject object, jfieldID field);

        /**
         * <code>FieldModification</code> event callbacks.
         *
         * @param jvmti        - the JVMTI interface pointer
         * @param jni          - the JNI interface pointer
         * @param thread       - the Java thread-generating event
         * @param method       - the modified-field method 
         * @param location     - the <code>FieldModification</code> event location 
         * @param field_class  - the modified-field class 
         * @param object       - the modified-field owner object
         * @param field        - the accessed-field ID 
         * @param sig          - the reference-type signature of a new field value
         * @param value        - a new value
         */
        static void JNICALL HandleFieldModification(jvmtiEnv* jvmti,
            JNIEnv* jni, jthread thread, jmethodID method, jlocation location,
            jclass field_class, jobject object, jfieldID field,
            char sig, jvalue value);

        /**
         * <code>SingleStep</code> event callbacks.
         *
         * @param jvmti     - the JVMTI interface pointer
         * @param jni       - the JNI interface pointer
         * @param thread    - the Java thread-generating event
         * @param method    - the single-step occurred method 
         * @param location  - the single-step occurred location
         */
        static void JNICALL HandleSingleStep(jvmtiEnv* jvmti, JNIEnv* jni,
            jthread thread, jmethodID method, jlocation location);

        /**
         * <code>FramePop</code> event callbacks.
         *
         * @param jvmti                    - the JVMTI interface pointer
         * @param jni                      - the JNI interface pointer
         * @param thread                   - the Java thread-generating event
         * @param method                   - the ID of popped method
         * @param was_popped_by_exception  - whether the exception-popped frame
         *                                   or normal return occurred
         */
        static void JNICALL HandleFramePop(jvmtiEnv* jvmti, JNIEnv* jni,
            jthread thread, jmethodID method, jboolean was_popped_by_exception);

    private:

        void ControlBreakpoint(JNIEnv* jni, AgentEventRequest* request,
            bool enable) throw(AgentException);

        void ControlWatchpoint(JNIEnv* jni, AgentEventRequest* request,
            bool enable) throw(AgentException);

        void ControlEvent(JNIEnv* jni, AgentEventRequest* request, bool enable)
            throw(AgentException);

        RequestList& GetRequestList(jdwpEventKind kind)
            throw(AgentException);

        void DeleteStepRequest(JNIEnv* jni, jthread thread)
            throw(AgentException);

        StepRequest* FindStepRequest(JNIEnv* jni, jthread thread)
            throw(AgentException);

        void GenerateEvents(
            JNIEnv* jni,
            EventInfo &event,
            jint &eventCount,
            RequestID* &eventList,
            jdwpSuspendPolicy &sp
        ) throw(AgentException);

        RequestID m_requestIdCount;
        AgentMonitor* m_requestMonitor;

        RequestList m_singleStepRequests;
        RequestList m_breakpointRequests;
        RequestList m_framePopRequests;
        RequestList m_exceptionRequests;
        RequestList m_userDefinedRequests;
        RequestList m_threadStartRequests;
        RequestList m_threadEndRequests;
        RequestList m_classPrepareRequests;
        RequestList m_classUnloadRequests;
        RequestList m_classLoadRequests;
        RequestList m_fieldAccessRequests;
        RequestList m_fieldModificationRequests;
        RequestList m_exceptionCatchRequests;
        RequestList m_methodEntryRequests;
        RequestList m_methodExitRequests;
        RequestList m_vmDeathRequests;

    };

}

#endif // _REQUEST_MANAGER_H_