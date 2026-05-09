#pragma once
#include "pch.h"
#include <string>

/*
 * JvmWrapper — Finds the JVM in the process, attaches our thread,
 * and provides cached JNI lookups with safety checks.
 *
 * Primary Strategy: JVMTI GetLoadedClasses (Iterates all classes, bypassing classloader issues).
 * Fallback Strategy: Game ClassLoader (Resolves classes using Forge's internal loader).
 */

struct jvmtiEnv_;
typedef jvmtiEnv_ jvmtiEnv;

class JvmWrapper {
public:
    static bool init();
    static void shutdown();

    static JNIEnv* getEnv();
    static JavaVM* getVM();

    // ── Class lookup (JVMTI -> ClassLoader -> FindClass) ───────────
    static jclass findClass(const char* jniName);

    // ── Method/field lookups ────────────────────────────────────────
    static jmethodID getMethodID(jclass cls, const char* name, const char* sig);
    static jmethodID getStaticMethodID(jclass cls, const char* name, const char* sig);
    static jfieldID  getFieldID(jclass cls, const char* name, const char* sig);
    static jfieldID  getStaticFieldID(jclass cls, const char* name, const char* sig);

    // ── Helpers ─────────────────────────────────────────────────────
    static std::string jstringToString(jstring str);
    static bool        checkException();

    // ── Diagnostics ─────────────────────────────────────────────────
    static void dumpClassInfo(jclass cls, const char* label, int maxMethods = 30, int maxFields = 20);
    static void dumpAllClasses(const char* filter = nullptr);

private:
    static bool bootstrapClassLoader(JNIEnv* env);
    static jmethodID findMethodViaJVMTI(JNIEnv* env, jclass cls, const char* name, bool isStatic);
    static jfieldID  findFieldViaJVMTI(JNIEnv* env, jclass cls, const char* name, bool isStatic);

    static inline JavaVM*     s_vm          = nullptr;
    static inline jvmtiEnv*   s_jvmti       = nullptr;
    static inline jobject     s_classLoader = nullptr;
    static inline jmethodID   s_loadClass   = nullptr;
    static inline std::unordered_map<std::string, jclass> s_classCache;
};
