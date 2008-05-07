/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
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
 * @author Viacheslav G. Rybalov
 * @version $Revision: 1.13 $
 */
// SocketTransport.cpp
//

/**
 * This is implementation of JDWP Agent TCP/IP Socket transport.
 * Main module.
 */

#ifndef USING_VMI
#define USING_VMI
#include "SocketTransport_pd.h"
#include "j9socket.h"
#include "j9sock.h"

typedef struct PortlibPTBuffers_struct
{
  struct PortlibPTBuffers_struct *next;	      /**< Next per thread buffer */
  struct PortlibPTBuffers_struct *previous;   /**< Previous per thread buffer */
  I_32 platformErrorCode;		      /**< error code as reported by the OS */
  I_32 portableErrorCode;		      /**< error code translated to portable format by application */
  char *errorMessageBuffer;		      /**< last saved error message, either customized or from OS */
  U_32 errorMessageBufferSize;		      /**< error message buffer size */
  I_32 reportedErrorCode;		      /**< last reported error code */
  char *reportedMessageBuffer;		      /**< last reported error message, either customized or from OS */
  U_32 reportedMessageBufferSize;	      /**< reported message buffer size */
  j9fdset_t fdset;			      /**< file descriptor set */
  j9addrinfo_struct addr_info_hints;
} PortlibPTBuffers_struct;

typedef struct PortlibPTBuffers_struct *PortlibPTBuffers_t;

extern void *VMCALL j9port_tls_get (struct HyPortLibrary *portLibrary);


/**
 * Returns the error status for the last failed operation. 
 */
static int
GetLastErrorStatus(jdwpTransportEnv* env)
{
    internalEnv* ienv = (internalEnv*)env->functions->reserved1;
    PORT_ACCESS_FROM_JAVAVM(ienv->jvm);
    return j9error_last_error_number();
} // GetLastErrorStatus

/**
 * Retrieves the number of milliseconds, substitute for the corresponding Win32 
 * function.
 */
static long 
GetTickCount(jdwpTransportEnv* env)
{
    internalEnv* ienv = (internalEnv*)env->functions->reserved1;
    PORT_ACCESS_FROM_JAVAVM(ienv->jvm);
    return (long)j9time_current_time_millis();
} // GetTickCount

/**
 * Initializes critical section lock objects.
 */
static inline void
InitializeCriticalSections(jdwpTransportEnv* env)
{
    internalEnv* ienv = (internalEnv*)env->functions->reserved1;
    j9thread_attach(NULL);

    UDATA flags = 0;
    if (j9thread_monitor_init(&(ienv->readLock), 1) != 0) {
	printf("initial error\n");
    }

    if (j9thread_monitor_init(&(ienv->sendLock), 1) != 0) {
	printf("initial error\n");
    }
    
} //InitializeCriticalSections()

/**
 * Releases all resources used by critical-section lock objects.
 */
static inline void
DeleteCriticalSections(jdwpTransportEnv* env)
{
    internalEnv* ienv = (internalEnv*)env->functions->reserved1;

    j9thread_attach(NULL);
    j9thread_monitor_destroy(ienv->readLock);
    j9thread_monitor_destroy(ienv->sendLock);
} //DeleteCriticalSections()

/**
 * Waits for ownership of the send critical-section object.
 */
static inline void
EnterCriticalSendSection(jdwpTransportEnv* env)
{
    internalEnv* ienv = (internalEnv*)env->functions->reserved1;

    j9thread_attach(NULL);
    j9thread_monitor_enter(ienv->sendLock);
} //EnterCriticalSendSection()

/**
 * Waits for ownership of the read critical-section object.
 */
static inline void
EnterCriticalReadSection(jdwpTransportEnv* env)
{
    internalEnv* ienv = (internalEnv*)env->functions->reserved1;

    j9thread_attach(NULL);
    j9thread_monitor_enter(ienv->readLock);
} //EnterCriticalReadSection()

/**
 * Releases ownership of the read critical-section object.
 */
static inline void
LeaveCriticalReadSection(jdwpTransportEnv* env)
{
    internalEnv* ienv = (internalEnv*)env->functions->reserved1;

    j9thread_attach(NULL);
    j9thread_monitor_exit(ienv->readLock);
} //LeaveCriticalReadSection()

/**
 * Releases ownership of the send critical-section object.
 */
static inline void
LeaveCriticalSendSection(jdwpTransportEnv* env)
{
    internalEnv* ienv = (internalEnv*)env->functions->reserved1;

     j9thread_attach(NULL);
     j9thread_monitor_exit(ienv->sendLock);
} //LeaveCriticalSendSection()


/**
 * This function sets into internalEnv struct message and status code of last transport error
 */
static void 
SetLastTranError(jdwpTransportEnv* env, const char* messagePtr, int errorStatus)
{
    internalEnv* ienv = (internalEnv*)env->functions->reserved1;
    if (ienv->lastError != 0) {
        ienv->lastError->insertError(messagePtr, errorStatus);
    } else {
	JNIEnv *jni;
	ienv->jvm->GetEnv((void **)&jni, JNI_VERSION_1_4);
        ienv->lastError = new(ienv->alloc, ienv->free) LastTransportError(jni, messagePtr, errorStatus, ienv->alloc, ienv->free);
    }
    return;
} // SetLastTranError

/**
 * This function sets into internalEnv struct prefix message for last transport error
 */
static void 
SetLastTranErrorMessagePrefix(jdwpTransportEnv* env, const char* messagePrefix)
{
    internalEnv* ienv = (internalEnv*)env->functions->reserved1;
    if (ienv->lastError != 0) {
        ienv->lastError->addErrorMessagePrefix(messagePrefix);
    }
    return;
} // SetLastTranErrorMessagePrefix


/**
 * The timeout used for invocation of select function in SelectRead and SelectSend methods
 */
static const jint cycle = 1000; // wait cycle in milliseconds 

/**
 * This function enable/disables socket blocking mode 
 */
static bool 
SetSocketBlockingMode(jdwpTransportEnv* env, j9socket_t sckt, bool isBlocked)
{
    JavaVM *vm = ((internalEnv*)env->functions->reserved1)->jvm;
    PORT_ACCESS_FROM_JAVAVM(vm);

    jint ret = j9sock_set_nonblocking(sckt, isBlocked ? FALSE : TRUE);
    if (ret != 0){
	SetLastTranError(env, "socket error", GetLastErrorStatus(env));
        return false;
    }
    return true;

} // SetSocketBlockingMode()

/**
 * This function is used to determine the read status of socket (in terms of select function).
 * The function avoids absolutely blocking select
 */
static jdwpTransportError 
SelectRead(jdwpTransportEnv* env, j9socket_t sckt, jlong deadline = 0) {
    internalEnv* ienv = (internalEnv*)env->functions->reserved1;
    PORT_ACCESS_FROM_JAVAVM(ienv->jvm);
/*
#ifdef WIN32
    SOCKET socket = sckt->ipv4;
#else
    SOCKET socket = sckt->sock;
#endif
    deadline = deadline == 0 ? 1000 : deadline;
    if (deadline >= 0) {
        TIMEVAL tv = {(long)(deadline / 1000), (long)(deadline % 1000)};
        fd_set fdread;
        FD_ZERO(&fdread);
        FD_SET(socket, &fdread);

        int ret = select((int)(socket) + 1, &fdread,NULL, NULL, &tv);
        if (ret < 0) {
            int err = GetLastErrorStatus(env);
            // ignore signal interruption
            if (err != SOCKET_ERROR_EINTR) {
                SetLastTranError(env, "socket error", err);
                return JDWPTRANSPORT_ERROR_IO_ERROR;
            }
        }
        if ((ret > 0) && (FD_ISSET(socket, &fdread))) {
            return JDWPTRANSPORT_ERROR_NONE; //timeout is not occurred
        }
    }
    SetLastTranError(env, "timeout occurred", 0);
    return JDWPTRANSPORT_ERROR_TIMEOUT; //timeout occurred
*/
    jint ret = j9sock_select_read(sckt, (I_32) deadline / 1000 , (I_32) deadline % 1000, FALSE);
    if (ret == 1){
	return JDWPTRANSPORT_ERROR_NONE; //timeout is not occurred
    }
    if (ret != J9PORT_ERROR_SOCKET_TIMEOUT){
    	 SetLastTranError(env, "socket error", ret);
         return JDWPTRANSPORT_ERROR_IO_ERROR;
    }
    SetLastTranError(env, "timeout occurred", 0);
    return JDWPTRANSPORT_ERROR_TIMEOUT; //timeout occurred

/*    jlong currentTimeout = cycle;
    while ((deadline == 0) || ((currentTimeout = (deadline - GetTickCount())) > 0)) {
        currentTimeout = currentTimeout < cycle ? currentTimeout : cycle;
        TIMEVAL tv = {(long)(currentTimeout / 1000), (long)(currentTimeout % 1000)};
        fd_set fdread;
        FD_ZERO(&fdread);
        FD_SET(sckt, &fdread);

        int ret = select((int)sckt + 1, &fdread, NULL, NULL, &tv);
        if (ret == SOCKET_ERROR) {
            int err = GetLastErrorStatus(env);
            // ignore signal interruption
            if (err != SOCKET_ERROR_EINTR) {
                SetLastTranError(env, "socket error", err);
                return JDWPTRANSPORT_ERROR_IO_ERROR;
            }
        }
        if ((ret > 0) && (FD_ISSET(sckt, &fdread))) {
            return JDWPTRANSPORT_ERROR_NONE; //timeout is not occurred
        }
    }
    SetLastTranError(env, "timeout occurred", 0);
    return JDWPTRANSPORT_ERROR_TIMEOUT; //timeout occurred
*/
} // SelectRead

/**
 * This function is used to determine the send status of socket (in terms of select function).
 * The function avoids absolutely blocking select
 */
static jdwpTransportError 
SelectSend(jdwpTransportEnv* env, j9socket_t sckt, jlong deadline = 0) {
    JavaVM *vm = ((internalEnv*)env->functions->reserved1)->jvm;
    PORT_ACCESS_FROM_JAVAVM(vm);

    j9fdset_struct j9fdSet;
    
    I_32 secTime = (long)(deadline / 1000);
    I_32 uTime = (long)(deadline % 1000);

    j9timeval_struct timeval;

    j9sock_fdset_zero(&j9fdSet);
    j9sock_fdset_set(sckt,&j9fdSet);

    int ret = j9sock_timeval_init(secTime,uTime,&timeval);

    ret =  j9sock_select(j9sock_fdset_size(sckt),NULL,&j9fdSet,NULL,&timeval);

    if (ret > 0){
        return JDWPTRANSPORT_ERROR_NONE; //timeout is not occurred
    }
    if (ret != J9PORT_ERROR_SOCKET_TIMEOUT){
    	 SetLastTranError(env, "socket error", ret);
         return JDWPTRANSPORT_ERROR_IO_ERROR;
    }
    SetLastTranError(env, "timeout occurred", 0);
    return JDWPTRANSPORT_ERROR_TIMEOUT; //timeout occurred

    //jlong currentTimeout = cycle;

// leave a workaround here, wait for new portlib for select APIs
/* #ifdef WIN32
    SOCKET socket = sckt->ipv4;
#else
    SOCKET socket = sckt->sock;
#endif
    deadline = deadline == 0 ? 100 : deadline;
    if (deadline >= 0) {
        TIMEVAL tv = {(long)(deadline / 1000), (long)(deadline % 1000)};
        fd_set fdwrite;
        FD_ZERO(&fdwrite);
        FD_SET(socket, &fdwrite);

        int ret = select((int)(socket) + 1, NULL, &fdwrite, NULL, &tv);
        if (ret < 0) {
            int err = GetLastErrorStatus(env);
            // ignore signal interruption
            if (err != SOCKET_ERROR_EINTR) {
                SetLastTranError(env, "socket error", err);
                return JDWPTRANSPORT_ERROR_IO_ERROR;
            }
        }
        if ((ret > 0) && (FD_ISSET(socket, &fdwrite))) {
            return JDWPTRANSPORT_ERROR_NONE; //timeout is not occurred
        }
    }
    SetLastTranError(env, "timeout occurred", 0);
    return JDWPTRANSPORT_ERROR_TIMEOUT; //timeout occurred
    */
} // SelectSend

/**
 * This function sends data on a connected socket
 */
static jdwpTransportError
SendData(jdwpTransportEnv* env, j9socket_t sckt, const char* data, int dataLength, jlong deadline = 0)
{
    long left = dataLength;
    long off = 0;
    int ret;

    JavaVM *vm = ((internalEnv*)env->functions->reserved1)->jvm;
    PORT_ACCESS_FROM_JAVAVM(vm);

    // Check if block
    while (left > 0){
        jdwpTransportError err = SelectSend(env, sckt, deadline);
        if (err != JDWPTRANSPORT_ERROR_NONE) {
            return err;
        }

	ret = j9sock_write (sckt, (U_8 *)data+off, left, J9SOCK_NOFLAGS);
	if (ret < 0){
                SetLastTranError(env, "socket error", ret);
                return JDWPTRANSPORT_ERROR_IO_ERROR; 
	}
	left -= ret;
	off += ret;
    }    
    return JDWPTRANSPORT_ERROR_NONE;
                                                                                                   
    /*
    while (left > 0) {
        jdwpTransportError err = SelectSend(env, sckt, deadline);
        if (err != JDWPTRANSPORT_ERROR_NONE) {
            return err;
        }
        ret = send(sckt, (data + off), left, 0);
        if (ret == SOCKET_ERROR) {
            int err = GetLastErrorStatus(env);
            // ignore signal interruption
            if (err != SOCKET_ERROR_EINTR) {
                SetLastTranError(env, "socket error", err);
                return JDWPTRANSPORT_ERROR_IO_ERROR;
            }
        }
        left -= ret;
        off += ret;
    } //while
    return JDWPTRANSPORT_ERROR_NONE;*/
} //SendData

/**
 * This function receives data from a connected socket
 */
static jdwpTransportError
ReceiveData(jdwpTransportEnv* env, j9socket_t sckt, U_8 * buffer, int dataLength, jlong deadline = 0, int* readByte = 0)
{
    JavaVM *vm = ((internalEnv*)env->functions->reserved1)->jvm;
    PORT_ACCESS_FROM_JAVAVM(vm);

    long left = dataLength;
    long off = 0;
    int ret;

    if (readByte != 0) {
        *readByte = 0;
    }

    while (left > 0) {
        jdwpTransportError err = SelectRead(env, sckt, deadline);
        if (err != JDWPTRANSPORT_ERROR_NONE) {
            return err;
        }

 	ret = j9sock_read(sckt, (U_8 *) (buffer + off), left, J9SOCK_NOFLAGS);

        if (ret < 0) {
            SetLastTranError(env, "data receiving failed", ret);
            return JDWPTRANSPORT_ERROR_IO_ERROR;
        }
        if (ret == 0) {
            SetLastTranError(env, "premature EOF", J9SOCK_NOFLAGS);
            return JDWPTRANSPORT_ERROR_IO_ERROR;
        }
        left -= ret;
        off += ret;
        if (readByte != 0) {
            *readByte = off;
        }
    } //while
    return JDWPTRANSPORT_ERROR_NONE;

/*
    while (left > 0) {
        jdwpTransportError err = SelectRead(env, sckt, deadline);
        if (err != JDWPTRANSPORT_ERROR_NONE) {
            return err;
        }
        ret = recv(sckt, (buffer + off), left, 0);
        if (ret == SOCKET_ERROR) {
            int err = GetLastErrorStatus(env);
            // ignore signal interruption
            if (err != SOCKET_ERROR_EINTR) {
                SetLastTranError(env, "data receiving failed", err);
                return JDWPTRANSPORT_ERROR_IO_ERROR;
            }
        }
        if (ret == 0) {
            SetLastTranError(env, "premature EOF", 0);
            return JDWPTRANSPORT_ERROR_IO_ERROR;
        }
        left -= ret;
        off += ret;
        if (readByte != 0) {
            *readByte = off;
        }
    } //while
    return JDWPTRANSPORT_ERROR_NONE;*/
} // ReceiveData

/**
 * This function performes handshake procedure
 */
static jdwpTransportError 
CheckHandshaking(jdwpTransportEnv* env, j9socket_t sckt, jlong handshakeTimeout)
{
    const char* handshakeString = "JDWP-Handshake";
    U_8 receivedString[14]; //length of "JDWP-Handshake"

    jdwpTransportError err;
    err = SendData(env, sckt, handshakeString, (int)strlen(handshakeString), handshakeTimeout);
    if (err != JDWPTRANSPORT_ERROR_NONE) {
        SetLastTranErrorMessagePrefix(env, "'JDWP-Handshake' sending error: ");
        return err;
    }
 
    err = ReceiveData(env, sckt, receivedString, (int)strlen(handshakeString), handshakeTimeout);
 
    if (err != JDWPTRANSPORT_ERROR_NONE) {
        SetLastTranErrorMessagePrefix(env, "'JDWP-Handshake' receiving error: ");
        return err;
    }
    if (memcmp(receivedString, handshakeString, 14) != 0) {
        SetLastTranError(env, "handshake error, 'JDWP-Handshake' is not received", 0);
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }
    return JDWPTRANSPORT_ERROR_NONE;
}// CheckHandshaking

/**
 * This function decodes address and populates sockaddr_in structure
 */
static jdwpTransportError
DecodeAddress(jdwpTransportEnv* env, const char *address, j9sockaddr_t sa, bool isServer) 
{
    JavaVM *vm = ((internalEnv*)env->functions->reserved1)->jvm;
    PORT_ACCESS_FROM_JAVAVM(vm);
    char * localhost = "127.0.0.1";
    char * anyhost = "0.0.0.0";
//    memset(sa, 0, sizeof(struct sockaddr_in));
//    sa->sin_family = AF_INET;

    if ((address == 0) || (*address == 0)) {  //empty address
        j9sock_sockaddr(sa,  isServer ? anyhost : localhost, 0);
//        sa->sin_addr.s_addr = isServer ? htonl(INADDR_ANY) : inet_addr("127.0.0.1");
//        sa->sin_port = 0;
        return JDWPTRANSPORT_ERROR_NONE;
    }

    const char* colon = strchr(address, ':');
    if (colon == 0) {  //address is like "port"
	j9sock_sockaddr(sa,  isServer ? anyhost : localhost,  j9sock_htons((U_16)atoi(address)));
        //sa->sin_port = htons((u_short)atoi(address));
        //sa->sin_addr.s_addr = isServer ? htonl(INADDR_ANY) : inet_addr("127.0.0.1");
    } else { //address is like "host:port"
        //sa->sin_port = htons((u_short)atoi(colon + 1));
        char *hostName = (char*)(((internalEnv*)env->functions->reserved1)
            ->alloc)((jint)(colon - address + 1));
        if (hostName == 0) {
            SetLastTranError(env, "out of memory", 0);
            return JDWPTRANSPORT_ERROR_OUT_OF_MEMORY;
        }
	memcpy(hostName, address, colon - address);
        hostName[colon - address] = '\0';
	int ret = j9sock_sockaddr(sa,  hostName, j9sock_htons((U_16)atoi(colon + 1)));
	if (ret != 0){
                SetLastTranError(env, "unable to resolve host name", 0);
                (((internalEnv*)env->functions->reserved1)->free)(hostName);
                return JDWPTRANSPORT_ERROR_IO_ERROR;
	}
        /* sa->sin_addr.s_addr = inet_addr(hostName);
        if (ret != 0) {
            struct hostent *host = gethostbyname(hostName);
            if (host == 0) {
                SetLastTranError(env, "unable to resolve host name", 0);
                (((internalEnv*)env->functions->reserved1)->free)(hostName);
                return JDWPTRANSPORT_ERROR_IO_ERROR;
            }
            //TODO delete this memcpy(&(sa->sin_addr), host->h_addr_list[0], host->h_length);
        } //if*/
        (((internalEnv*)env->functions->reserved1)->free)(hostName);
    } //if
    return JDWPTRANSPORT_ERROR_NONE;
} //DecodeAddress

/**
 * This function implements jdwpTransportEnv::GetCapabilities
 */
static jdwpTransportError JNICALL
TCPIPSocketTran_GetCapabilities(jdwpTransportEnv* env, 
        JDWPTransportCapabilities* capabilitiesPtr) 
{
    memset(capabilitiesPtr, 0, sizeof(JDWPTransportCapabilities));
    capabilitiesPtr->can_timeout_attach = 1;
    capabilitiesPtr->can_timeout_accept = 1;       
    capabilitiesPtr->can_timeout_handshake = 1;
    return JDWPTRANSPORT_ERROR_NONE;
} //TCPIPSocketTran_GetCapabilities

/**
 * This function implements jdwpTransportEnv::Close
 */
static jdwpTransportError JNICALL 
TCPIPSocketTran_Close(jdwpTransportEnv* env)
{  
    JavaVM *vm = ((internalEnv*)env->functions->reserved1)->jvm;
    PORT_ACCESS_FROM_JAVAVM(vm);
    j9socket_t envClientSocket = ((internalEnv*)env->functions->reserved1)->envClientSocket;
    if (envClientSocket == NULL) {
        return JDWPTRANSPORT_ERROR_NONE;
    }

    ((internalEnv*)env->functions->reserved1)->envClientSocket = NULL;
    if (j9sock_socketIsValid(envClientSocket)==0){
        return JDWPTRANSPORT_ERROR_NONE;
    }

    int err;
    err = j9sock_shutdown_input(envClientSocket);
    if (err == 0){
	 err = j9sock_shutdown_output(envClientSocket);
    }
    if (err != 0) {
        SetLastTranError(env, "shutdown socket failed", GetLastErrorStatus(env));
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }
/*#ifdef WIN32
    SOCKET socket = envClientSocket->ipv4;
    err = closesocket(socket);
#else
    SOCKET socket = envClientSocket->sock;
    err = close(socket);
#endif*/ 
    err = j9sock_close(&envClientSocket);

    if (err != 0) {
        SetLastTranError(env, "close socket failed", GetLastErrorStatus(env));
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }
    return JDWPTRANSPORT_ERROR_NONE;

/*    err = shutdown(envClientSocket, SD_BOTH);
    if (err == SOCKET_ERROR) {
        SetLastTranError(env, "close socket failed", GetLastErrorStatus(env));
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }

    err = closesocket(envClientSocket);
    if (err == SOCKET_ERROR) {
        SetLastTranError(env, "close socket failed", GetLastErrorStatus(env));
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }

    return JDWPTRANSPORT_ERROR_NONE;*/
} //TCPIPSocketTran_Close

/**
 * This function sets socket options SO_REUSEADDR and TCP_NODELAY
 */
static bool 
SetSocketOptions(jdwpTransportEnv* env, j9socket_t sckt) 
{
    JavaVM *vm = ((internalEnv*)env->functions->reserved1)->jvm;
    PORT_ACCESS_FROM_JAVAVM(vm);

    BOOLEAN isOn = TRUE;

    if (j9sock_setopt_bool(sckt, J9_SOL_SOCKET, J9_SO_REUSEADDR, &isOn) != 0){
        SetLastTranError(env, "setsockopt(SO_REUSEADDR) failed", GetLastErrorStatus(env));
        return false;
    }
    if (j9sock_setopt_bool(sckt, J9_IPPROTO_IP, J9_TCP_NODELAY,  &isOn) != 0) {
        SetLastTranError(env, "setsockopt(TCPNODELAY) failed", GetLastErrorStatus(env));
        return false;
    }

    return true;

/*    if (setsockopt(sckt, SOL_SOCKET, SO_REUSEADDR, (const char*)&isOn, sizeof(isOn)) == SOCKET_ERROR) {                                                              
        SetLastTranError(env, "setsockopt(SO_REUSEADDR) failed", GetLastErrorStatus(env));
        return false;
    }
    if (setsockopt(sckt, IPPROTO_TCP, TCP_NODELAY, (const char*)&isOn, sizeof(isOn)) == SOCKET_ERROR) {
        SetLastTranError(env, "setsockopt(TCPNODELAY) failed", GetLastErrorStatus(env));
        return false;
    }
    return true;*/
} // SetSocketOptions()

/**
 * This function implements jdwpTransportEnv::Attach
 */
static jdwpTransportError JNICALL 
TCPIPSocketTran_Attach(jdwpTransportEnv* env, const char* address,
        jlong attachTimeout, jlong handshakeTimeout)
{
    internalEnv *ienv = (internalEnv*)env->functions->reserved1;
    PORT_ACCESS_FROM_JAVAVM(ienv->jvm);

    j9socket_t clientSocket;  
    j9sockaddr_struct serverSockAddr;  
                                   
    if ((address == 0) || (*address == 0)) {
        SetLastTranError(env, "address is missing", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_ARGUMENT;
    }

    if (attachTimeout < 0) {
        SetLastTranError(env, "attachTimeout timeout is negative", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_ARGUMENT;
    }

    if (handshakeTimeout < 0) {
        SetLastTranError(env, "handshakeTimeout timeout is negative", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_ARGUMENT;
    }

    j9socket_t envClientSocket = ((internalEnv*)env->functions->reserved1)->envClientSocket;
    if (envClientSocket != NULL) {
        SetLastTranError(env, "there is already an open connection to the debugger", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_STATE ;
    }

    j9socket_t envServerSocket = ((internalEnv*)env->functions->reserved1)->envServerSocket;
    if (envServerSocket != NULL) {
        SetLastTranError(env, "transport is currently in listen mode", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_STATE ;
    }

    jdwpTransportError res = DecodeAddress(env, address, &serverSockAddr, false);
    if (res != JDWPTRANSPORT_ERROR_NONE) {
        return res;
    }

    int ret = j9sock_socket(&clientSocket,J9SOCK_AFINET, J9SOCK_STREAM, J9SOCK_DEFPROTOCOL);
	   // socket(AF_INET, SOCK_STREAM, 0);
    if (ret != 0) {
        SetLastTranError(env, "unable to create socket", GetLastErrorStatus(env));
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }
    
    if (!SetSocketOptions(env, clientSocket)) {
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }

    if (attachTimeout == 0) {
        if (!SetSocketBlockingMode(env, clientSocket, true)) {
            return JDWPTRANSPORT_ERROR_IO_ERROR;
        }
        int err = j9sock_connect(clientSocket, &serverSockAddr);
	//int err = connect(clientSocket, (struct sockaddr *)&serverSockAddr, sizeof(serverSockAddr));
	if (err != 0 ) {
            SetLastTranError(env, "connection failed", GetLastErrorStatus(env));
            SetSocketBlockingMode(env, clientSocket, false);
            return JDWPTRANSPORT_ERROR_IO_ERROR;
        }  
        if (!SetSocketBlockingMode(env, clientSocket, false)) {
            return JDWPTRANSPORT_ERROR_IO_ERROR;
	}
    } else {
        if (!SetSocketBlockingMode(env, clientSocket, false)) {
            return JDWPTRANSPORT_ERROR_IO_ERROR;
        }
        int err = j9sock_connect(clientSocket, &serverSockAddr);
        if (err != 0) {
            if (err != J9PORT_ERROR_SOCKET_WOULDBLOCK) {
                SetLastTranError(env, "connection failed", GetLastErrorStatus(env));
                return JDWPTRANSPORT_ERROR_IO_ERROR;
            } else {
                int ret = SelectSend(env, clientSocket, handshakeTimeout);
		if (ret == JDWPTRANSPORT_ERROR_NONE){
			return JDWPTRANSPORT_ERROR_NONE;
		}
                return JDWPTRANSPORT_ERROR_IO_ERROR; 
//TODO delele this selectWrite
                /*fd_set fdwrite;
                FD_ZERO(&fdwrite);
                FD_SET(clientSocket, &fdwrite);
                TIMEVAL tv = {(long)(attachTimeout / 1000), (long)(attachTimeout % 1000)};

                int ret = select((int)clientSocket + 1, NULL, &fdwrite, NULL, &tv);
                if (ret == SOCKET_ERROR) {
                    SetLastTranError(env, "socket error", GetLastErrorStatus(env));
                    return JDWPTRANSPORT_ERROR_IO_ERROR;
                }
                if ((ret != 1) || !(FD_ISSET(clientSocket, &fdwrite))) {
                    SetLastTranError(env, "timeout occurred", 0);
                    return JDWPTRANSPORT_ERROR_IO_ERROR;
                }*/
            }
        }
    }
    EnterCriticalSendSection(env);
    EnterCriticalReadSection(env);
    ((internalEnv*)env->functions->reserved1)->envClientSocket = clientSocket;
    res = CheckHandshaking(env, clientSocket, (long)handshakeTimeout);
    LeaveCriticalReadSection(env);
    LeaveCriticalSendSection(env);
    if (res != JDWPTRANSPORT_ERROR_NONE) {
        TCPIPSocketTran_Close(env);
        return res;
    }
    return JDWPTRANSPORT_ERROR_NONE;
} //TCPIPSocketTran_Attach

/**
 * This function implements jdwpTransportEnv::StartListening
 */
static jdwpTransportError JNICALL 
TCPIPSocketTran_StartListening(jdwpTransportEnv* env, const char* address, 
        char** actualAddress)
{
    JavaVM *vm = ((internalEnv*)env->functions->reserved1)->jvm;
    PORT_ACCESS_FROM_JAVAVM(vm);

    j9socket_t envClientSocket = ((internalEnv*)env->functions->reserved1)->envClientSocket;
    if (envClientSocket != NULL) {
        SetLastTranError(env, "there is already an open connection to the debugger", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_STATE ;
    }

    j9socket_t envServerSocket = ((internalEnv*)env->functions->reserved1)->envServerSocket;
    if (envServerSocket != NULL) {
        SetLastTranError(env, "transport is currently in listen mode", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_STATE ;
    }

    jdwpTransportError res;
    j9sockaddr_struct serverSockAddr;
    res = DecodeAddress(env, address, &serverSockAddr, true);
    if (res != JDWPTRANSPORT_ERROR_NONE) {
        return res;
    }

    j9socket_t serverSocket;
    int ret = j9sock_socket(&serverSocket,J9SOCK_AFINET, J9SOCK_STREAM, J9SOCK_DEFPROTOCOL); //socket(AF_INET, SOCK_STREAM, 0);
    if (ret != 0) {
        SetLastTranError(env, "unable to create socket", GetLastErrorStatus(env));
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }

    if (!SetSocketOptions(env, serverSocket)) {
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }

    int err;

    err = j9sock_bind (serverSocket, &serverSockAddr);
    // bind(serverSocket, (struct sockaddr *)&serverSockAddr, sizeof(serverSockAddr));
    if (err != 0 ) {
        SetLastTranError(env, "binding to port failed", GetLastErrorStatus(env));
        return JDWPTRANSPORT_ERROR_ILLEGAL_STATE ;
    }

    err = j9sock_listen(serverSocket, J9SOCK_MAXCONN);
    if (err != 0) {
        SetLastTranError(env, "listen start failed", GetLastErrorStatus(env));
        return JDWPTRANSPORT_ERROR_ILLEGAL_STATE ;
    }

    if (!SetSocketBlockingMode(env, serverSocket, false)) {
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }

    ((internalEnv*)env->functions->reserved1)->envServerSocket = serverSocket;

    err = j9sock_getsockname(serverSocket, &serverSockAddr);
    if (err != 0) {
        SetLastTranError(env, "socket error", GetLastErrorStatus(env));
        return JDWPTRANSPORT_ERROR_ILLEGAL_STATE ;
    }

    char* retAddress = 0;

    // RI always returns only port number in listening mode
/*
    char portName[6];
    sprintf(portName, "%d", ntohs(serverSockAddr.sin_port)); //instead of itoa()

    char hostName[NI_MAXHOST];
    if (getnameinfo((struct sockaddr *)&serverSockAddr, len, hostName, sizeof(hostName), NULL, 0, 0)) {
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }
    if (strcmp(hostName, "0.0.0.0") == 0) {
        gethostname(hostName, sizeof(hostName));
    }
    retAddress = (char*)(((internalEnv*)env->functions->reserved1)
        ->alloc)((jint)(strlen(hostName) + strlen(portName) + 2)); 
    if (retAddress == 0) {
        SetLastTranError(env, "out of memory", 0);
        return JDWPTRANSPORT_ERROR_OUT_OF_MEMORY;
    }
    sprintf(retAddress, "%s:%s", hostName, portName);
*/
    retAddress = (char*)(((internalEnv*)env->functions->reserved1)->alloc)(6 + 1); 
    if (retAddress == 0) {
        SetLastTranError(env, "out of memory", 0);
        return JDWPTRANSPORT_ERROR_OUT_OF_MEMORY;
    }
    // print server port
    sprintf(retAddress, "%d",j9sock_sockaddr_port(&serverSockAddr));

    *actualAddress = retAddress;

    return JDWPTRANSPORT_ERROR_NONE;
} //TCPIPSocketTran_StartListening

/**
 * This function implements jdwpTransportEnv::StopListening
 */
static jdwpTransportError JNICALL 
TCPIPSocketTran_StopListening(jdwpTransportEnv* env)
{
    JavaVM *vm = ((internalEnv*)env->functions->reserved1)->jvm;
    PORT_ACCESS_FROM_JAVAVM(vm);

    j9socket_t envServerSocket = ((internalEnv*)env->functions->reserved1)->envServerSocket;
    if (envServerSocket == NULL) {
        return JDWPTRANSPORT_ERROR_NONE;
    }
/*#ifdef WIN32
    SOCKET socket = envServerSocket->ipv4;
    int err = closesocket(socket);
#else
    SOCKET socket = envServerSocket->sock;
    int err = close(socket);
#endif */
    int err = j9sock_close(&envServerSocket);
    if (err != 0) {
        SetLastTranError(env, "close socket failed", GetLastErrorStatus(env));
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }

    ((internalEnv*)env->functions->reserved1)->envServerSocket = NULL;

    return JDWPTRANSPORT_ERROR_NONE;
} //TCPIPSocketTran_StopListening

/**
 * This function implements jdwpTransportEnv::Accept
 */
static jdwpTransportError JNICALL 
TCPIPSocketTran_Accept(jdwpTransportEnv* env, jlong acceptTimeout,
        jlong handshakeTimeout)
{
    JavaVM *vm = ((internalEnv*)env->functions->reserved1)->jvm;
    PORT_ACCESS_FROM_JAVAVM(vm);

    if (acceptTimeout < 0) {
        SetLastTranError(env, "acceptTimeout timeout is negative", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_ARGUMENT;
    }

    if (handshakeTimeout < 0) {
        SetLastTranError(env, "handshakeTimeout timeout is negative", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_ARGUMENT;
    }

    j9socket_t envClientSocket = ((internalEnv*)env->functions->reserved1)->envClientSocket;
    if (envClientSocket != NULL) {
        SetLastTranError(env, "there is already an open connection to the debugger", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_STATE ;
    }

    j9socket_t envServerSocket = ((internalEnv*)env->functions->reserved1)->envServerSocket;
    if (envServerSocket == NULL) {
        SetLastTranError(env, "transport is not currently in listen mode", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_STATE ;
    }

    j9sockaddr_struct serverSockAddr;
/*    int res = j9sock_getpeername(envServerSocket, &serverSockAddr);
    if (res == SOCKET_ERROR) {
        SetLastTranError(env, "connection failed", GetLastErrorStatus(env));
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }*/

    //jlong deadline = (acceptTimeout == 0) ? 0 : (jlong)GetTickCount() + acceptTimeout;
    I_32 ret = SelectRead(env, envServerSocket, acceptTimeout);
    //I_32 ret = j9sock_select_read(envServerSocket, (I_32)acceptTimeout/1000, (I_32)acceptTimeout%1000, TRUE);

    if (ret != JDWPTRANSPORT_ERROR_NONE){
        if (ret != J9PORT_ERROR_SOCKET_TIMEOUT){
             SetLastTranError(env, "socket error", ret);
             return JDWPTRANSPORT_ERROR_IO_ERROR;
        }
        SetLastTranError(env, "timeout occurred", 0);
        return JDWPTRANSPORT_ERROR_TIMEOUT; //timeout occurred
    }

    j9socket_t clientSocket;
    ret = j9sock_accept(envServerSocket, &serverSockAddr, &clientSocket);

    if (ret != 0) {
        SetLastTranError(env, "socket accept failed", GetLastErrorStatus(env));
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }

    if (!SetSocketBlockingMode(env, clientSocket, false)) {
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    }

    EnterCriticalSendSection(env);
    EnterCriticalReadSection(env);
    ((internalEnv*)env->functions->reserved1)->envClientSocket = clientSocket;
    jdwpTransportError err = CheckHandshaking(env, clientSocket, (long)handshakeTimeout);
    LeaveCriticalReadSection(env);
    LeaveCriticalSendSection(env);
    if (err != JDWPTRANSPORT_ERROR_NONE) {
        TCPIPSocketTran_Close(env);
        return err;
    }
    return JDWPTRANSPORT_ERROR_NONE;
} //TCPIPSocketTran_Accept

/**
 * This function implements jdwpTransportEnv::IsOpen
 */
static jboolean JNICALL 
TCPIPSocketTran_IsOpen(jdwpTransportEnv* env)
{
    j9socket_t envClientSocket = ((internalEnv*)env->functions->reserved1)->envClientSocket;
    if (envClientSocket == NULL) {
        return JNI_FALSE;
    }
    return JNI_TRUE;
} //TCPIPSocketTran_IsOpen

/**
 * This function read packet
 */
static jdwpTransportError
ReadPacket(jdwpTransportEnv* env, j9socket_t envClientSocket, jdwpPacket* packet)
{
    JavaVM *vm = ((internalEnv*)env->functions->reserved1)->jvm;
    PORT_ACCESS_FROM_JAVAVM(vm);

    jdwpTransportError err;
    int length;
    int readBytes = 0;
    err = ReceiveData(env, envClientSocket, (U_8 *)&length, sizeof(jint), 0, &readBytes);
    if (err != JDWPTRANSPORT_ERROR_NONE) {
        if (readBytes == 0) {
            packet->type.cmd.len = 0;
            return JDWPTRANSPORT_ERROR_NONE;
        }
        return err;
    }
    packet->type.cmd.len = (jint)j9sock_ntohl(length);
    
    int id;
    err = ReceiveData(env, envClientSocket, (U_8 *)&(id), sizeof(jint));
    if (err != JDWPTRANSPORT_ERROR_NONE) {
        return err;
    }

    packet->type.cmd.id = (jint)j9sock_ntohl(id);

    err = ReceiveData(env, envClientSocket, (U_8 *)&(packet->type.cmd.flags), sizeof(jbyte));
    if (err != JDWPTRANSPORT_ERROR_NONE) {
        return err;
    }

    if (packet->type.cmd.flags & JDWPTRANSPORT_FLAGS_REPLY) {
        int errorCode;
        err = ReceiveData(env, envClientSocket, (U_8*)&(errorCode), sizeof(jshort));
        if (err != JDWPTRANSPORT_ERROR_NONE) {
            return err;
        }
        packet->type.reply.errorCode = (jshort)j9sock_ntohs(errorCode); 
    } else {
        err = ReceiveData(env, envClientSocket, (U_8*)&(packet->type.cmd.cmdSet), sizeof(jbyte));
        if (err != JDWPTRANSPORT_ERROR_NONE) {
            return err;
        }
 
        err = ReceiveData(env, envClientSocket, (U_8*)&(packet->type.cmd.cmd), sizeof(jbyte));
        if (err != JDWPTRANSPORT_ERROR_NONE) {
            return err;
        }
    } //if

    int dataLength = packet->type.cmd.len - 11;
    if (dataLength < 0) {
        SetLastTranError(env, "invalid packet length received", 0);
        return JDWPTRANSPORT_ERROR_IO_ERROR;
    } else if (dataLength == 0) {
        packet->type.cmd.data = 0;
    } else {
        packet->type.cmd.data = (jbyte*)(((internalEnv*)env->functions->reserved1)->alloc)(dataLength);
        if (packet->type.cmd.data == 0) {
            SetLastTranError(env, "out of memory", 0);
            return JDWPTRANSPORT_ERROR_OUT_OF_MEMORY;
        }
        err = ReceiveData(env, envClientSocket, (U_8 *)packet->type.cmd.data, dataLength);
        if (err != JDWPTRANSPORT_ERROR_NONE) {
            (((internalEnv*)env->functions->reserved1)->free)(packet->type.cmd.data);
            return err;
        }
    } //if
    return JDWPTRANSPORT_ERROR_NONE;
}

/**
 * This function implements jdwpTransportEnv::ReadPacket
 */
static jdwpTransportError JNICALL 
TCPIPSocketTran_ReadPacket(jdwpTransportEnv* env, jdwpPacket* packet)
{
    if (packet == 0) {
        SetLastTranError(env, "packet is 0", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_ARGUMENT;
    }

    j9socket_t envClientSocket = ((internalEnv*)env->functions->reserved1)->envClientSocket;
    if (envClientSocket == NULL) {
        SetLastTranError(env, "there isn't an open connection to a debugger", 0);
        LeaveCriticalReadSection(env);
        return JDWPTRANSPORT_ERROR_ILLEGAL_STATE ;
    }

    EnterCriticalReadSection(env);
    jdwpTransportError err = ReadPacket(env, envClientSocket, packet);
    LeaveCriticalReadSection(env);
    return JDWPTRANSPORT_ERROR_NONE;
} //TCPIPSocketTran_ReadPacket

/**
 * This function implements jdwpTransportEnv::WritePacket
 */
static jdwpTransportError 
WritePacket(jdwpTransportEnv* env, j9socket_t envClientSocket, const jdwpPacket* packet)
{
    JavaVM *vm = ((internalEnv*)env->functions->reserved1)->jvm;
    PORT_ACCESS_FROM_JAVAVM(vm);
    int packetLength = packet->type.cmd.len;
    if (packetLength < 11) {
        SetLastTranError(env, "invalid packet length", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_ARGUMENT;
    }

    char* data = (char*)packet->type.cmd.data;
    if ((packetLength > 11) && (data == 0)) {
        SetLastTranError(env, "packet length is greater than 11 but the packet data field is 0", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_ARGUMENT;
    }

    int dataLength = packetLength - 11;
    packetLength = j9sock_htonl(packetLength);

    jdwpTransportError err;
    err = SendData(env, envClientSocket, (char*)&packetLength, sizeof(jint));
    if (err != JDWPTRANSPORT_ERROR_NONE) {
        return err;
    }

    int id = j9sock_htonl (packet->type.cmd.id);

    err = SendData(env, envClientSocket, (char*)&id, sizeof(jint));
    if (err != JDWPTRANSPORT_ERROR_NONE) {
        return err;
    }

    err = SendData(env, envClientSocket, (char*)&(packet->type.cmd.flags), sizeof(jbyte));
    if (err != JDWPTRANSPORT_ERROR_NONE) {
        return err;
    }

    if (packet->type.cmd.flags & JDWPTRANSPORT_FLAGS_REPLY) {
        int errorCode = htons(packet->type.reply.errorCode);
        err = SendData(env, envClientSocket, (char*)&errorCode, sizeof(jshort));
        if (err != JDWPTRANSPORT_ERROR_NONE) {
            return err;
        }
    } else {
        err = SendData(env, envClientSocket, (char*)&(packet->type.cmd.cmdSet), sizeof(jbyte));
        if (err != JDWPTRANSPORT_ERROR_NONE) {
            return err;
        }
        err = SendData(env, envClientSocket, (char*)&(packet->type.cmd.cmd), sizeof(jbyte));
        if (err != JDWPTRANSPORT_ERROR_NONE) {
            return err;
        }
    } //if
    
    if (data != 0) {
        err = SendData(env, envClientSocket, data, dataLength);
        if (err != JDWPTRANSPORT_ERROR_NONE) {
            return err;
        }
    } //if
    return JDWPTRANSPORT_ERROR_NONE;
}

/**
 * This function send packet
 */
static jdwpTransportError JNICALL 
TCPIPSocketTran_WritePacket(jdwpTransportEnv* env, const jdwpPacket* packet)
{

    if (packet == 0) {
        SetLastTranError(env, "packet is 0", 0);
        return JDWPTRANSPORT_ERROR_ILLEGAL_ARGUMENT;
    }

    j9socket_t envClientSocket = ((internalEnv*)env->functions->reserved1)->envClientSocket;
    if (envClientSocket == NULL) {
        SetLastTranError(env, "there isn't an open connection to a debugger", 0);
        //LeaveCriticalSendSection(env);
        return JDWPTRANSPORT_ERROR_ILLEGAL_STATE;
    }

    EnterCriticalSendSection(env);
    jdwpTransportError err = WritePacket(env, envClientSocket, packet);
    LeaveCriticalSendSection(env);
    return err;
} //TCPIPSocketTran_WritePacket

/**
 * This function implements jdwpTransportEnv::GetLastError
 */
static jdwpTransportError JNICALL 
TCPIPSocketTran_GetLastError(jdwpTransportEnv* env, char** message)
{
    *message = ((internalEnv*)env->functions->reserved1)->lastError->GetLastErrorMessage();

    if (*message == 0) {
        return JDWPTRANSPORT_ERROR_MSG_NOT_AVAILABLE;
    }
    return JDWPTRANSPORT_ERROR_NONE;
} //TCPIPSocketTran_GetLastError

/**
 * This function must be called by agent when the library is loaded
 */
extern "C" JNIEXPORT jint JNICALL 
jdwpTransport_OnLoad(JavaVM *vm, jdwpTransportCallback* callback,
             jint version, jdwpTransportEnv** env)
{
    if (version != JDWPTRANSPORT_VERSION_1_0) {
        return JNI_EVERSION;
    }

    internalEnv* iEnv = (internalEnv*)callback->alloc(sizeof(internalEnv));
    if (iEnv == 0) {
        return JNI_ENOMEM;
    }
    iEnv->jvm = vm;
    iEnv->alloc = callback->alloc;
    iEnv->free = callback->free;
    iEnv->lastError = 0;
    iEnv->envClientSocket = NULL;
    iEnv->envServerSocket = NULL;

    jdwpTransportNativeInterface_* envTNI = (jdwpTransportNativeInterface_*)callback
        ->alloc(sizeof(jdwpTransportNativeInterface_));
    if (envTNI == 0) {
        callback->free(iEnv);
        return JNI_ENOMEM;
    }

    envTNI->GetCapabilities = &TCPIPSocketTran_GetCapabilities;
    envTNI->Attach = &TCPIPSocketTran_Attach;
    envTNI->StartListening = &TCPIPSocketTran_StartListening;
    envTNI->StopListening = &TCPIPSocketTran_StopListening;
    envTNI->Accept = &TCPIPSocketTran_Accept;
    envTNI->IsOpen = &TCPIPSocketTran_IsOpen;
    envTNI->Close = &TCPIPSocketTran_Close;
    envTNI->ReadPacket = &TCPIPSocketTran_ReadPacket;
    envTNI->WritePacket = &TCPIPSocketTran_WritePacket;
    envTNI->GetLastError = &TCPIPSocketTran_GetLastError;
    envTNI->reserved1 = iEnv;

    _jdwpTransportEnv* resEnv = (_jdwpTransportEnv*)callback
        ->alloc(sizeof(_jdwpTransportEnv));
    if (resEnv == 0) {
        callback->free(iEnv);
        callback->free(envTNI);
        return JNI_ENOMEM;
    }

    resEnv->functions = envTNI;
    *env = resEnv;

    InitializeCriticalSections(resEnv);

    return JNI_OK;
} //jdwpTransport_OnLoad

/**
 * This function may be called by agent before the library unloading.
 * The function is not defined in JDWP Transport Interface specification.
 */
extern "C" JNIEXPORT void JNICALL 
jdwpTransport_UnLoad(jdwpTransportEnv** env)
{
    DeleteCriticalSections(*env);
    TCPIPSocketTran_Close(*env);
    TCPIPSocketTran_StopListening(*env);
    void (*unLoadFree)(void *buffer) = ((internalEnv*)(*env)->functions->reserved1)->free;
    if (((internalEnv*)(*env)->functions->reserved1)->lastError != 0){
        delete (((internalEnv*)(*env)->functions->reserved1)->lastError);
    }
    unLoadFree((void*)(*env)->functions->reserved1);
    unLoadFree((void*)(*env)->functions);
    unLoadFree((void*)(*env));
} //jdwpTransport_UnLoad
#endif
