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
 * @author Aleksander V. Budniy
 * @version $Revision: 1.3 $
 */

/**
 * Created on 12.08.2005
 */
package org.apache.harmony.jpda.tests.jdwp.MultiSession;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

/**
 * This class provides simple debuggee class used sync connection.
 */
public class ConnectorKindDebuggee extends SyncDebuggee {

    public void run() {
    int i=0;
        while(true) {   
            synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
            
            logWriter.println("Hello World ---- "+i);
            i++;
            
            String message = synchronizer.receiveMessage();
            if (!message.equals(JPDADebuggeeSynchronizer.SGNL_CONTINUE)) {
                break;
            }
            
        }
        synchronizer.sendMessage("END");
        logWriter.println("Debuggee ended");
    }

    public static void main(String [] args) {
        runDebuggee(ConnectorKindDebuggee.class);
    }
}