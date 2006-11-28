/*
 * Copyright 2005-2006 The Apache Software Foundation or its licensors, as applicable.
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
 * @author Vitaly A. Provodin
 * @version $Revision: 1.5 $
 */

/**
 * Created on 29.01.2005
 */
package org.apache.harmony.jpda.tests.jdwp.share;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;

/**
 * Base class for unit tests which use one debuggee VM with synchronization
 * channel.
 */
public abstract class JDWPSyncTestCase extends JDWPTestCase {

    protected JPDADebuggeeSynchronizer synchronizer;

    protected String finalSyncMessage;

    /**
     * This method is invoked right before starting debuggee VM.
     */
    protected void beforeDebuggeeStart(JDWPUnitDebuggeeWrapper debuggeeWrapper) {
        synchronizer = new JPDADebuggeeSynchronizer(logWriter, settings);
        int port = synchronizer.bindServer();
        debuggeeWrapper.savedVMOptions = "-Djpda.settings.syncPort=" + port;
        super.beforeDebuggeeStart(debuggeeWrapper);
    }

    /**
     * Overrides inherited method to resume debuggee VM and then to establish
     * sync connection.
     */
    protected void internalSetUp() throws Exception {
        super.internalSetUp();

        debuggeeWrapper.resume();
        logWriter.println("Resumed debuggee VM");

        synchronizer.startServer();
        logWriter.println("Established sync connection");
    }

    /**
     * Overrides inherited method to close sync connection upon exit.
     */
    protected void internalTearDown() {
        if (synchronizer != null) {
            if (null != finalSyncMessage) {
                synchronizer.sendMessage(finalSyncMessage);
            }
            synchronizer.stop();
            logWriter.println("Completed sync connection");
        }
        super.internalTearDown();
    }

}