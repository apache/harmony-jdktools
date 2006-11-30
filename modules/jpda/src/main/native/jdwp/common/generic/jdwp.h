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
 * @version $Revision: 1.5.2.1 $
 */

/**
 * @file
 * jdwp.h
 *
 */

#ifndef _JDWP_H_
#define _JDWP_H_

/* JDWP Version */
#define JDWP_VERSION_MAJOR 1
#define JDWP_VERSION_MINOR 5

/* General JDWP constants */
#define JDWP_FLAG_REPLY_PACKET ((jbyte)0x80)
#define JDWP_MIN_PACKET_LENGTH 11

/* Command Sets */
typedef enum jdwpCommandSet {
    JDWP_COMMAND_SET_VIRTUAL_MACHINE = 1,
    JDWP_COMMAND_SET_REFERENCE_TYPE = 2,
    JDWP_COMMAND_SET_CLASS_TYPE = 3,
    JDWP_COMMAND_SET_ARRAY_TYPE = 4,
    JDWP_COMMAND_SET_INTERFACE_TYPE = 5,
    JDWP_COMMAND_SET_METHOD = 6,
    JDWP_COMMAND_SET_FIELD = 8,
    JDWP_COMMAND_SET_OBJECT_REFERENCE = 9,
    JDWP_COMMAND_SET_STRING_REFERENCE = 10,
    JDWP_COMMAND_SET_THREAD_REFERENCE = 11,
    JDWP_COMMAND_SET_THREAD_GROUP_REFERENCE = 12,
    JDWP_COMMAND_SET_ARRAY_REFERENCE = 13,
    JDWP_COMMAND_SET_CLASS_LOADER_REFERENCE = 14,
    JDWP_COMMAND_SET_EVENT_REQUEST = 15,
    JDWP_COMMAND_SET_STACK_FRAME = 16,
    JDWP_COMMAND_SET_CLASS_OBJECT_REFERENCE = 17,
    JDWP_COMMAND_SET_EVENT = 64
} jdwpCommandSet;


typedef enum jdwpCommand {

    /* Commands VirtualMachine */
    JDWP_COMMAND_VM_VERSION = 1,
    JDWP_COMMAND_VM_CLASSES_BY_SIGNATURE = 2,
    JDWP_COMMAND_VM_ALL_CLASSES = 3,
    JDWP_COMMAND_VM_ALL_THREADS = 4,
    JDWP_COMMAND_VM_TOP_LEVEL_THREAD_GROUPS = 5,
    JDWP_COMMAND_VM_DISPOSE = 6,
    JDWP_COMMAND_VM_ID_SIZES = 7,
    JDWP_COMMAND_VM_SUSPEND = 8,
    JDWP_COMMAND_VM_RESUME = 9,
    JDWP_COMMAND_VM_EXIT = 10,
    JDWP_COMMAND_VM_CREATE_STRING = 11,
    JDWP_COMMAND_VM_CAPABILITIES = 12,
    JDWP_COMMAND_VM_CLASS_PATHS = 13,
    JDWP_COMMAND_VM_DISPOSE_OBJECTS = 14,
    JDWP_COMMAND_VM_HOLD_EVENTS = 15,
    JDWP_COMMAND_VM_RELEASE_EVENTS = 16,
    JDWP_COMMAND_VM_CAPABILITIES_NEW = 17,
    JDWP_COMMAND_VM_REDEFINE_CLASSES = 18,
    JDWP_COMMAND_VM_SET_DEFAULT_STRATUM = 19,
    JDWP_COMMAND_VM_ALL_CLASSES_WITH_GENERIC= 20,

    /* Commands ReferenceType */
    JDWP_COMMAND_RT_SIGNATURE = 1,
    JDWP_COMMAND_RT_CLASS_LOADER = 2,
    JDWP_COMMAND_RT_MODIFIERS = 3,
    JDWP_COMMAND_RT_FIELDS = 4,
    JDWP_COMMAND_RT_METHODS = 5,
    JDWP_COMMAND_RT_GET_VALUES = 6,
    JDWP_COMMAND_RT_SOURCE_FILE = 7,
    JDWP_COMMAND_RT_NESTED_TYPES = 8,
    JDWP_COMMAND_RT_STATUS = 9,
    JDWP_COMMAND_RT_INTERFACES = 10,
    JDWP_COMMAND_RT_CLASS_OBJECT = 11,
    JDWP_COMMAND_RT_SOURCE_DEBUG_EXTENSION = 12,
    JDWP_COMMAND_RT_SIGNATURE_WITH_GENERIC = 13,
    JDWP_COMMAND_RT_FIELDS_WITH_GENERIC = 14,
    JDWP_COMMAND_RT_METHODS_WITH_GENERIC = 15,

    /* Commands ClassType */
    JDWP_COMMAND_CT_SUPERCLASS = 1,
    JDWP_COMMAND_CT_SET_VALUES = 2,
    JDWP_COMMAND_CT_INVOKE_METHOD = 3,
    JDWP_COMMAND_CT_NEW_INSTANCE = 4,

    /* Commands ArrayType */
    JDWP_COMMAND_AT_NEW_INSTANCE = 1,

    /* Commands Method */
    JDWP_COMMAND_M_LINE_TABLE = 1,
    JDWP_COMMAND_M_VARIABLE_TABLE = 2,
    JDWP_COMMAND_M_BYTECODES = 3,
    JDWP_COMMAND_M_OBSOLETE = 4,
    JDWP_COMMAND_M_VARIABLE_TABLE_WITH_GENERIC = 5,

    /* Commands ObjectReference */
    JDWP_COMMAND_OR_REFERENCE_TYPE = 1,
    JDWP_COMMAND_OR_GET_VALUES = 2,
    JDWP_COMMAND_OR_SET_VALUES = 3,
    JDWP_COMMAND_OR_MONITOR_INFO = 5,
    JDWP_COMMAND_OR_INVOKE_METHOD = 6,
    JDWP_COMMAND_OR_DISABLE_COLLECTION = 7,
    JDWP_COMMAND_OR_ENABLE_COLLECTION = 8,
    JDWP_COMMAND_OR_IS_COLLECTED = 9,

    /* Commands StringReference */
    JDWP_COMMAND_SR_VALUE = 1,

    /* Commands ThreadReference */
    JDWP_COMMAND_TR_NAME = 1,
    JDWP_COMMAND_TR_SUSPEND = 2,
    JDWP_COMMAND_TR_RESUME = 3,
    JDWP_COMMAND_TR_STATUS = 4,
    JDWP_COMMAND_TR_THREAD_GROUP = 5,
    JDWP_COMMAND_TR_FRAMES = 6,
    JDWP_COMMAND_TR_FRAME_COUNT = 7,
    JDWP_COMMAND_TR_OWNED_MONITORS = 8,
    JDWP_COMMAND_TR_CURRENT_CONTENDED_MONITOR = 9,
    JDWP_COMMAND_TR_STOP = 10,
    JDWP_COMMAND_TR_INTERRUPT = 11,
    JDWP_COMMAND_TR_SUSPEND_COUNT = 12,

    /* Commands ThreadGroupReference */
    JDWP_COMMAND_TGR_NAME = 1,
    JDWP_COMMAND_TGR_PARENT = 2,
    JDWP_COMMAND_TGR_CHILDREN = 3,

    /* Commands ArrayReference */
    JDWP_COMMAND_AR_LENGTH = 1,
    JDWP_COMMAND_AR_GET_VALUES = 2,
    JDWP_COMMAND_AR_SET_VALUES = 3,

    /* Commands ClassLoaderReference */
    JDWP_COMMAND_CLR_VISIBLE_CLASSES = 1,

    /* Commands EventRequest */
    JDWP_COMMAND_ER_SET = 1,
    JDWP_COMMAND_ER_CLEAR = 2,
    JDWP_COMMAND_ER_CLEAR_ALL_BREAKPOINTS = 3,

    /* Commands StackFrame */
    JDWP_COMMAND_SF_GET_VALUES = 1,
    JDWP_COMMAND_SF_SET_VALUES = 2,
    JDWP_COMMAND_SF_THIS_OBJECT = 3,
    JDWP_COMMAND_SF_POP_FRAME = 4,

    /* Commands ClassObjectReference */
    JDWP_COMMAND_COR_REFLECTED_TYPE = 1,

    /* Commands Event */
    JDWP_COMMAND_E_COMPOSITE = 100

} jdwpCommand;


/* Error Constants */
typedef enum jdwpError {
    JDWP_ERROR_NONE = 0,
    JDWP_ERROR_INVALID_THREAD = 10,
    JDWP_ERROR_INVALID_THREAD_GROUP = 11,
    JDWP_ERROR_INVALID_PRIORITY = 12,
    JDWP_ERROR_THREAD_NOT_SUSPENDED = 13,
    JDWP_ERROR_THREAD_SUSPENDED = 14,
    JDWP_ERROR_INVALID_OBJECT = 20,
    JDWP_ERROR_INVALID_CLASS = 21,
    JDWP_ERROR_CLASS_NOT_PREPARED = 22,
    JDWP_ERROR_INVALID_METHODID = 23,
    JDWP_ERROR_INVALID_LOCATION = 24,
    JDWP_ERROR_INVALID_FIELDID = 25,
    JDWP_ERROR_INVALID_FRAMEID = 30,
    JDWP_ERROR_NO_MORE_FRAMES = 31,
    JDWP_ERROR_OPAQUE_FRAME = 32,
    JDWP_ERROR_NOT_CURRENT_FRAME = 33,
    JDWP_ERROR_TYPE_MISMATCH = 34,
    JDWP_ERROR_INVALID_SLOT = 35,
    JDWP_ERROR_DUPLICATE = 40,
    JDWP_ERROR_NOT_FOUND = 41,
    JDWP_ERROR_INVALID_MONITOR = 50,
    JDWP_ERROR_NOT_MONITOR_OWNER = 51,
    JDWP_ERROR_INTERRUPT = 52,
    JDWP_ERROR_INVALID_CLASS_FORMAT = 60,
    JDWP_ERROR_CIRCULAR_CLASS_DEFINITION = 61,
    JDWP_ERROR_FAILS_VERIFICATION = 62,
    JDWP_ERROR_ADD_METHOD_NOT_IMPLEMENTED = 63,
    JDWP_ERROR_SCHEMA_CHANGE_NOT_IMPLEMENTED = 64,
    JDWP_ERROR_INVALID_TYPESTATE = 65,
    JDWP_ERROR_HIERARCHY_CHANGE_NOT_IMPLEMENTED = 66,
    JDWP_ERROR_DELETE_METHOD_NOT_IMPLEMENTED = 67,
    JDWP_ERROR_UNSUPPORTED_VERSION = 68,
    JDWP_ERROR_NAMES_DONT_MATCH = 69,
    JDWP_ERROR_CLASS_MODIFIERS_CHANGE_NOT_IMPLEMENTED = 70,
    JDWP_ERROR_METHOD_MODIFIERS_CHANGE_NOT_IMPLEMENTED = 71,
    JDWP_ERROR_NOT_IMPLEMENTED = 99,
    JDWP_ERROR_NULL_POINTER = 100,
    JDWP_ERROR_ABSENT_INFORMATION = 101,
    JDWP_ERROR_INVALID_EVENT_TYPE = 102,
    JDWP_ERROR_ILLEGAL_ARGUMENT = 103,
    JDWP_ERROR_OUT_OF_MEMORY = 110,
    JDWP_ERROR_ACCESS_DENIED = 111,
    JDWP_ERROR_VM_DEAD = 112,
    JDWP_ERROR_INTERNAL = 113,
    JDWP_ERROR_UNATTACHED_THREAD = 115,
    JDWP_ERROR_INVALID_TAG = 500,
    JDWP_ERROR_ALREADY_INVOKING = 502,
    JDWP_ERROR_INVALID_INDEX = 503,
    JDWP_ERROR_INVALID_LENGTH = 504,
    JDWP_ERROR_INVALID_STRING = 506,
    JDWP_ERROR_INVALID_CLASS_LOADER = 507,
    JDWP_ERROR_INVALID_ARRAY = 508,
    JDWP_ERROR_TRANSPORT_LOAD = 509,
    JDWP_ERROR_TRANSPORT_INIT = 510,
    JDWP_ERROR_NATIVE_METHOD = 511,
    JDWP_ERROR_INVALID_COUNT = 512
} jdwpError;


/* EventKind Constants */
typedef enum jdwpEventKind {
    JDWP_EVENT_SINGLE_STEP = 1,
    JDWP_EVENT_BREAKPOINT = 2,
    JDWP_EVENT_FRAME_POP = 3,
    JDWP_EVENT_EXCEPTION = 4,
    JDWP_EVENT_USER_DEFINED = 5,
    JDWP_EVENT_THREAD_START = 6,
    JDWP_EVENT_THREAD_END = 7,
    JDWP_EVENT_THREAD_DEATH = JDWP_EVENT_THREAD_END,
    JDWP_EVENT_CLASS_PREPARE = 8,
    JDWP_EVENT_CLASS_UNLOAD = 9,
    JDWP_EVENT_CLASS_LOAD = 10,
    JDWP_EVENT_FIELD_ACCESS = 20,
    JDWP_EVENT_FIELD_MODIFICATION = 21,
    JDWP_EVENT_EXCEPTION_CATCH = 30,
    JDWP_EVENT_METHOD_ENTRY = 40,
    JDWP_EVENT_METHOD_EXIT = 41,
    JDWP_EVENT_VM_INIT = 90,
    JDWP_EVENT_VM_START = JDWP_EVENT_VM_INIT,
    JDWP_EVENT_VM_DEATH = 99,
    JDWP_EVENT_VM_DISCONNECTED = 100
} jdwpEventKind;

/* EventRequest/ModifierKind Constants */
typedef enum jdwpRequestModifier {
    JDWP_MODIFIER_NONE = 0,
    JDWP_MODIFIER_COUNT = 1,
    JDWP_MODIFIER_CONDITIONAL = 2,
    JDWP_MODIFIER_THREAD_ONLY = 3,
    JDWP_MODIFIER_CLASS_ONLY = 4,
    JDWP_MODIFIER_CLASS_MATCH = 5,
    JDWP_MODIFIER_CLASS_EXCLUDE = 6,
    JDWP_MODIFIER_LOCATION_ONLY = 7,
    JDWP_MODIFIER_EXCEPTION_ONLY = 8,
    JDWP_MODIFIER_FIELD_ONLY = 9,
    JDWP_MODIFIER_STEP = 10,
    JDWP_MODIFIER_INSTANCE_ONLY = 11
} jdwpRequestModifier;

/* ThreadStatus Constants */
typedef enum jdwpThreadStatus {
    JDWP_THREAD_STATUS_UNKNOWN = -1,
    JDWP_THREAD_STATUS_ZOMBIE = 0,
    JDWP_THREAD_STATUS_RUNNING = 1,
    JDWP_THREAD_STATUS_SLEEPING = 2,
    JDWP_THREAD_STATUS_MONITOR = 3,
    JDWP_THREAD_STATUS_WAIT = 4,
    JDWP_THREAD_STATUS_NOT_STARTED = 5
} jdwpThreadStatus;

/* SuspendStatus Constants */
#define JDWP_SUSPEND_STATUS_SUSPENDED 0x1

/* ClassStatus Constants */
typedef enum jdwpClassStatus {
    JDWP_CLASS_STATUS_VERIFIED = 1,
    JDWP_CLASS_STATUS_PREPARED = 2,
    JDWP_CLASS_STATUS_INITIALIZED = 4,
    JDWP_CLASS_STATUS_ERROR = 8
} jdwpClassStatus;

/* TypeTag Constants */
typedef enum jdwpTypeTag {
    JDWP_TYPE_TAG_CLASS = 1,
    JDWP_TYPE_TAG_INTERFACE = 2,
    JDWP_TYPE_TAG_ARRAY = 3
} jdwpTypeTag;

/* Tag Constants */
typedef enum jdwpTag {
    JDWP_TAG_NONE = 0,
    JDWP_TAG_ARRAY = 91,
    JDWP_TAG_BYTE = 66,
    JDWP_TAG_CHAR = 67,
    JDWP_TAG_OBJECT = 76,
    JDWP_TAG_FLOAT = 70,
    JDWP_TAG_DOUBLE = 68,
    JDWP_TAG_INT = 73,
    JDWP_TAG_LONG = 74,
    JDWP_TAG_SHORT = 83,
    JDWP_TAG_VOID = 86,
    JDWP_TAG_BOOLEAN = 90,
    JDWP_TAG_STRING = 115,
    JDWP_TAG_THREAD = 116,
    JDWP_TAG_THREAD_GROUP = 103,
    JDWP_TAG_CLASS_LOADER = 108,
    JDWP_TAG_CLASS_OBJECT = 99
} jdwpTag;

/* representation of null ObjectID */
#define JDWP_OBJECT_ID_NULL 0

/* StepDepth Constants */
typedef enum jdwpStepDepth {
    JDWP_STEP_INTO = 0,
    JDWP_STEP_OVER = 1,
    JDWP_STEP_OUT = 2
} jdwpStepDepth;

/* StepSize Constants */
typedef enum jdwpStepSize {
    JDWP_STEP_MIN = 0,
    JDWP_STEP_LINE = 1
} jdwpStepSize;

/* SuspendPolicy Constants */
typedef enum jdwpSuspendPolicy {
    JDWP_SUSPEND_NONE = 0,
    JDWP_SUSPEND_EVENT_THREAD = 1,
    JDWP_SUSPEND_ALL = 2
} jdwpSuspendPolicy;

/* Invoke options constants */
#define JDWP_INVOKE_SINGLE_THREADED 0x01
#define JDWP_INVOKE_NONVIRTUAL 0x02


#endif /* _JDWP_H_ */