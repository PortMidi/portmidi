#include "portmidi.h"
#include "jportmidi_JPortMidiApi.h"
#include <stdio.h>

/*
 * Method:    Pm_Initialize
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1Initialize
  (JNIEnv *env, jclass cl)
{
    return Pm_Initialize();
}


/*
 * Method:    Pm_Terminate
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1Terminate
  (JNIEnv *env, jclass cl)
{
    return Pm_Terminate();
}

/*
 * Method:    Pm_HasHostError
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1HasHostError
  (JNIEnv *env, jclass cl, jobject jstream)
{
    jclass c = (*env)->GetObjectClass(env, jstream);
    jfieldID fid = (*env)->GetFieldID(env, c, "address", "I");
    return Pm_HasHostError(
            (PmStream *) (*env)->GetIntField(env, jstream, fid));
}

/*
 * Method:    Pm_GetErrorText
 */
JNIEXPORT jstring JNICALL Java_jportmidi_JPortMidiApi_Pm_1GetErrorText
  (JNIEnv *env, jclass cl, jint i)
{
    return (*env)->NewStringUTF(env, Pm_GetErrorText(i));
}

/*
 * Method:    Pm_GetHostErrorText
 */
JNIEXPORT jstring JNICALL Java_jportmidi_JPortMidiApi_Pm_1GetHostErrorText
  (JNIEnv *env, jclass cl)
{
    char msg[PM_HOST_ERROR_MSG_LEN];
    Pm_GetHostErrorText(msg, PM_HOST_ERROR_MSG_LEN);
    return (*env)->NewStringUTF(env, msg);
}

/*
 * Method:    Pm_CountDevices
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1CountDevices
  (JNIEnv *env, jclass cl)
{
    printf("Pm_CountDevices called.\n");
    return Pm_CountDevices();
}

/*
 * Method:    Pm_GetDefaultInputDeviceID
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1GetDefaultInputDeviceID
  (JNIEnv *env, jclass cl)
{
    return Pm_GetDefaultInputDeviceID();
}

/*
 * Method:    Pm_GetDefaultOutputDeviceID
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1GetDefaultOutputDeviceID
  (JNIEnv *env, jclass cl)
{
    return Pm_GetDefaultOutputDeviceID();
}

/*
 * Method:    Pm_GetDeviceInterf
 */
JNIEXPORT jstring JNICALL Java_jportmidi_JPortMidiApi_Pm_1GetDeviceInterf
  (JNIEnv *env, jclass cl, jint i)
{
    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
    if (!info) return NULL;
    return (*env)->NewStringUTF(env, info->interf);
}

/*
 * Method:    Pm_GetDeviceName
 */
JNIEXPORT jstring JNICALL Java_jportmidi_JPortMidiApi_Pm_1GetDeviceName
  (JNIEnv *env, jclass cl, jint i)
{
    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
    if (!info) return NULL;
    printf("jni:Pm_GetDeviceName %s\n", info->name);
    return (*env)->NewStringUTF(env, info->name);
}

/*
 * Method:    Pm_GetDeviceInput
 */
JNIEXPORT jboolean JNICALL Java_jportmidi_JPortMidiApi_Pm_1GetDeviceInput
  (JNIEnv *env, jclass cl, jint i)
{
    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
    if (!info) return (jboolean) 0;
    return (jboolean) info->input;
}

/*
 * Method:    Pm_GetDeviceOutput
 */
JNIEXPORT jboolean JNICALL Java_jportmidi_JPortMidiApi_Pm_1GetDeviceOutput
  (JNIEnv *env, jclass cl, jint i)
{
    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
    if (!info) return (jboolean) 0;
    return (jboolean) info->output;
}

/*
 * Method:    Pm_OpenInput
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1OpenInput
  (JNIEnv *env, jclass cl, 
   jobject jstream, jint index, jstring extras, jint bufsiz)
{
    PmError rslt;
    PortMidiStream *stream;
    jclass c = (*env)->GetObjectClass(env, jstream);
    jfieldID fid = (*env)->GetFieldID(env, c, "address", "I");
    rslt = Pm_OpenInput(&stream, index, NULL, bufsiz, NULL, NULL);
    (*env)->SetIntField(env, jstream, fid, (int) stream);
    return rslt;
}

/*
 * Method:    Pm_OpenOutput
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1OpenOutput
  (JNIEnv *env, jclass cl, jobject jstream, jint index, jstring extras,
   jint bufsiz, jint latency)
{
    PmError rslt;
    PortMidiStream *stream;
    jclass c = (*env)->GetObjectClass(env, jstream);
    jfieldID fid = (*env)->GetFieldID(env, c, "address", "I");
    rslt = Pm_OpenOutput(&stream, index, NULL, bufsiz, NULL, NULL, latency);
    (*env)->SetIntField(env, jstream, fid, (int) stream);
    return rslt;
}

/*
 * Method:    Pm_SetFilter
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1SetFilter
  (JNIEnv *env, jclass cl, jobject jstream, jint filters)
{
    jclass c = (*env)->GetObjectClass(env, jstream);
    jfieldID fid = (*env)->GetFieldID(env, c, "address", "I");
    return Pm_SetFilter(
        (PmStream *) (*env)->GetIntField(env, jstream, fid), filters);
}

/*
 * Method:    Pm_SetChannelMask
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1SetChannelMask
  (JNIEnv *env, jclass cl, jobject jstream, jint mask)
{
    jclass c = (*env)->GetObjectClass(env, jstream);
    jfieldID fid = (*env)->GetFieldID(env, c, "address", "I");
    return Pm_SetChannelMask(
        (PmStream *) (*env)->GetIntField(env, jstream, fid), mask);
}

/*
 * Method:    Pm_Abort
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1Abort
  (JNIEnv *env, jclass cl, jobject jstream)
{
    jclass c = (*env)->GetObjectClass(env, jstream);
    jfieldID fid = (*env)->GetFieldID(env, c, "address", "I");
    return Pm_Abort((PmStream *) (*env)->GetIntField(env, jstream, fid));
}

/*
 * Method:    Pm_Close
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1Close
  (JNIEnv *env, jclass cl, jobject jstream)
{
    jclass c = (*env)->GetObjectClass(env, jstream);
    jfieldID fid = (*env)->GetFieldID(env, c, "address", "I");
    return Pm_Close((PmStream *) (*env)->GetIntField(env, jstream, fid));
}

/*
 * Method:    Pm_Read
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1Read
  (JNIEnv *env, jclass cl, jobject jstream, jobject jpmevent)
{
    jclass jstream_class = (*env)->GetObjectClass(env, jstream);
    jfieldID address_fid = 
            (*env)->GetFieldID(env, jstream_class, "address", "I");
    jclass jpmevent_class = (*env)->GetObjectClass(env, jpmevent);
    jfieldID message_fid = 
            (*env)->GetFieldID(env, jpmevent_class, "message", "I");
    jfieldID timestamp_fid = 
            (*env)->GetFieldID(env, jpmevent_class, "timestamp", "I");
    PmEvent buffer;
    PmError rslt = Pm_Read(
        (PmStream *) (*env)->GetIntField(env, jstream, address_fid), 
        &buffer, 1);
    (*env)->SetIntField(env, jpmevent, message_fid, buffer.message);
    (*env)->SetIntField(env, jpmevent, timestamp_fid, buffer.timestamp);
    return rslt;
}

/*
 * Method:    Pm_Poll
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1Poll
        (JNIEnv *env, jclass cl, jobject jstream)
{
    jclass c = (*env)->GetObjectClass(env, jstream);
    jfieldID fid = (*env)->GetFieldID(env, c, "address", "I");
    return Pm_Poll((PmStream *) (*env)->GetIntField(env, jstream, fid));
}

/*
 * Method:    Pm_Write
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1Write
        (JNIEnv *env, jclass cl, jobject jstream, jobject jpmevent)
{
    jclass jstream_class = (*env)->GetObjectClass(env, jstream);
    jfieldID address_fid = 
            (*env)->GetFieldID(env, jstream_class, "address", "I");
    jclass jpmevent_class = (*env)->GetObjectClass(env, jpmevent);
    jfieldID message_fid = 
            (*env)->GetFieldID(env, jpmevent_class, "message", "I");
    jfieldID timestamp_fid = 
            (*env)->GetFieldID(env, jpmevent_class, "timestamp", "I");
    // note that we call WriteShort because it's simpler than constructing
    // a buffer and passing it to Pm_Write
    return Pm_WriteShort(
        (PmStream *) (*env)->GetIntField(env, jstream, address_fid),
        (*env)->GetIntField(env, jpmevent, timestamp_fid),
        (*env)->GetIntField(env, jpmevent, message_fid));
}

/*
 * Method:    Pm_WriteShort
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1WriteShort
  (JNIEnv *env, jclass cl, jobject jstream, jint when, jint msg)
{
    jclass c = (*env)->GetObjectClass(env, jstream);
    jfieldID fid = (*env)->GetFieldID(env, c, "address", "I");
    return Pm_WriteShort(
        (PmStream *) (*env)->GetIntField(env, jstream, fid), when, msg);
}

/*
 * Method:    Pm_WriteSysEx
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pm_1WriteSysEx
  (JNIEnv *env, jclass cl, jobject jstream, jint when, jbyteArray jmsg)
{
    jclass c = (*env)->GetObjectClass(env, jstream);
    jfieldID fid = (*env)->GetFieldID(env, c, "address", "I");
    jsize len = (*env)->GetArrayLength(env, jmsg);
    jbyte *bytes = (*env)->GetByteArrayElements(env, jmsg, 0);
    PmError rslt = Pm_WriteSysEx(
        (PmStream *) (*env)->GetIntField(env, jstream, fid), when, 
                                         (unsigned char *) bytes);
    (*env)->ReleaseByteArrayElements(env, jmsg, bytes, 0);
    return rslt;
}

/*
 * Method:    Pt_TimeStart
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pt_1TimeStart
        (JNIEnv *env, jclass c, jint resolution)
{
    return Pt_Start(resolution, NULL, NULL);
}

/*
 * Method:    Pt_TimeStop
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pt_1TimeStop
        (JNIEnv *env, jclass c)
 {
     return Pt_Stop();
 }

/*
 * Method:    Pt_Time
 */
JNIEXPORT jint JNICALL Java_jportmidi_JPortMidiApi_Pt_1Time
        (JNIEnv *env, jclass c)
 {
     return Pt_Time();
 }

/*
 * Method:    Pt_TimeStarted
 */
JNIEXPORT jboolean JNICALL Java_jportmidi_JPortMidiApi_Pt_1TimeStarted
        (JNIEnv *env, jclass c)
{
    return Pt_Started();
}


