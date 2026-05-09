#include "pch.h"
#include "sdk/minecraft.h"

static constexpr bool kDumpClassDiagnostics = false;

bool CMinecraft::initIDs() {
    s_mcClass = JvmWrapper::findClass(Mappings::Minecraft_Class);
    if (!s_mcClass) {
        printf("[Ghost] ERROR: Minecraft class not found!\n");
        return false;
    }

    // ── Try to find getInstance method ──────────────────────────────
    s_getInstance = JvmWrapper::getStaticMethodID(s_mcClass, Mappings::MC_getInstance, Mappings::MC_getInstance_Sig);
    
    // ── Try to find the static 'instance' field directly ────────────
    // In Forge 1.21.1, the singleton may be stored in a field called 'instance'
    JNIEnv* env = JvmWrapper::getEnv();
    if (env) {
        // Try common field names for the singleton
        const char* fieldNames[] = { "instance", "INSTANCE" };
        for (auto fname : fieldNames) {
            if (s_instanceField) break;
            s_instanceField = env->GetStaticFieldID(s_mcClass, fname, Mappings::MC_getInstance_Sig);
            if (env->ExceptionCheck()) { env->ExceptionClear(); s_instanceField = nullptr; }
            if (s_instanceField) {
                printf("[Ghost] Found Minecraft singleton field: '%s'\n", fname);
            }
        }
        
        // If standard field lookup fails, try JVMTI scan for static fields of Minecraft type
        if (!s_instanceField) {
            s_instanceField = JvmWrapper::getStaticFieldID(s_mcClass, "instance", Mappings::MC_getInstance_Sig);
            if (s_instanceField) {
                printf("[Ghost] Found Minecraft singleton field via JVMTI: 'instance'\n");
            }
        }
    }
    
    s_player = JvmWrapper::getFieldID(s_mcClass, Mappings::MC_player, Mappings::MC_player_Sig);
    s_level  = JvmWrapper::getFieldID(s_mcClass, Mappings::MC_level, Mappings::MC_level_Sig);
    s_gameRenderer = JvmWrapper::getFieldID(s_mcClass, Mappings::MC_gameRenderer, Mappings::MC_gameRenderer_Sig);
    s_options = JvmWrapper::getFieldID(s_mcClass, Mappings::MC_options, Mappings::MC_options_Sig);

    // ── GameRenderer ───────────────────────────────────────────────────
    s_grClass = JvmWrapper::findClass(Mappings::GameRenderer_Class);
    if (s_grClass) {
        if (kDumpClassDiagnostics) {
            JvmWrapper::dumpClassInfo(s_grClass, "GameRenderer", 200, 50);
        }
        
        s_getMainCamera = JvmWrapper::getMethodID(s_grClass, Mappings::GR_getMainCamera, Mappings::GR_getMainCamera_Sig);
        s_getGRProjMatrix = JvmWrapper::getMethodID(s_grClass, Mappings::GR_getProjectionMatrix, Mappings::GR_getProjectionMatrix_Sig);
        s_getFov = JvmWrapper::getMethodID(s_grClass, Mappings::GR_getFov, Mappings::GR_getFov_Sig);
        
        if (s_getMainCamera) printf("[Ghost] GameRenderer::getMainCamera resolved OK\n");
        if (s_getGRProjMatrix) printf("[Ghost] GameRenderer::getProjectionMatrix resolved OK\n");
        if (s_getFov) printf("[Ghost] GameRenderer::getFov resolved OK\n");
        
        if (env->ExceptionCheck()) env->ExceptionClear();
    }

    // ── Camera (fields, not methods — Camera is NOT an Entity subclass) ──
    s_camClass = JvmWrapper::findClass(Mappings::Camera_Class);
    if (s_camClass) {
        if (kDumpClassDiagnostics) {
            JvmWrapper::dumpClassInfo(s_camClass, "Camera", 30, 20);
        }
        
        // Camera stores position/rotation as FIELDS
        s_camPosition = env->GetFieldID(s_camClass, "position", "Lnet/minecraft/world/phys/Vec3;");
        if (env->ExceptionCheck()) { env->ExceptionClear(); s_camPosition = nullptr; }
        
        s_camYRot = env->GetFieldID(s_camClass, "yRot", "F");
        if (env->ExceptionCheck()) { env->ExceptionClear(); s_camYRot = nullptr; }
        
        s_camXRot = env->GetFieldID(s_camClass, "xRot", "F");
        if (env->ExceptionCheck()) { env->ExceptionClear(); s_camXRot = nullptr; }
        
        bool camOK = s_camPosition && s_camYRot && s_camXRot;
        printf("[Ghost] Camera fields resolved: %s (pos=%p yRot=%p xRot=%p)\n",
               camOK ? "OK" : "PARTIAL", s_camPosition, s_camYRot, s_camXRot);
    } else {
        printf("[Ghost] WARN: Camera class not found\n");
    }

    // ── Options FOV ────────────────────────────────────────────────────
    s_optionsClass = JvmWrapper::findClass(Mappings::Options_Class);
    if (s_optionsClass && s_options) {
        // fov is an OptionInstance<Integer>
        s_optFov = env->GetFieldID(s_optionsClass, "fov",
            "Lnet/minecraft/client/OptionInstance;");
        if (env->ExceptionCheck()) { env->ExceptionClear(); s_optFov = nullptr; }
        
        if (s_optFov) {
            // OptionInstance.get() returns Object
            jclass optInstClass = JvmWrapper::findClass("net/minecraft/client/OptionInstance");
            if (optInstClass) {
                s_optInstGet = env->GetMethodID(optInstClass, "get", "()Ljava/lang/Object;");
                if (env->ExceptionCheck()) { env->ExceptionClear(); s_optInstGet = nullptr; }
                printf("[Ghost] Options FOV resolved: %s\n", s_optInstGet ? "OK" : "FAILED");
            }
        } else {
            printf("[Ghost] WARN: Options.fov field not found\n");
        }
    }

    // ── Window (framebuffer dimensions for aspect ratio) ─────────────
    s_window = env->GetFieldID(s_mcClass, "window",
        "Lcom/mojang/blaze3d/platform/Window;");
    if (env->ExceptionCheck()) { env->ExceptionClear(); s_window = nullptr; }

    if (s_window) {
        s_windowClass = JvmWrapper::findClass("com/mojang/blaze3d/platform/Window");
        if (s_windowClass) {
            s_getWidth  = env->GetMethodID(s_windowClass, "getWidth",  "()I");
            if (env->ExceptionCheck()) { env->ExceptionClear(); s_getWidth = nullptr; }
            s_getHeight = env->GetMethodID(s_windowClass, "getHeight", "()I");
            if (env->ExceptionCheck()) { env->ExceptionClear(); s_getHeight = nullptr; }
            printf("[Ghost] Window resolved: %s (w=%p h=%p)\n",
                   (s_getWidth && s_getHeight) ? "OK" : "PARTIAL", s_getWidth, s_getHeight);
        } else {
            printf("[Ghost] WARN: Window class not found\n");
        }
    } else {
        printf("[Ghost] WARN: Minecraft.window field not found\n");
    }

    // ── RenderSystem and Matrix4f ─────────────────────────────────────
    s_rsClass = JvmWrapper::findClass(Mappings::RenderSystem_Class);
    if (s_rsClass) {
        s_rsGetProj = env->GetStaticMethodID(s_rsClass,
            Mappings::RS_getProjectionMatrix, Mappings::RS_getProjectionMatrix_Sig);
        if (env->ExceptionCheck()) { env->ExceptionClear(); s_rsGetProj = nullptr; }

        s_rsGetModelView = env->GetStaticMethodID(s_rsClass,
            Mappings::RS_getModelViewMatrix, Mappings::RS_getModelViewMatrix_Sig);
        if (env->ExceptionCheck()) { env->ExceptionClear(); s_rsGetModelView = nullptr; }

        s_mat4Class = JvmWrapper::findClass(Mappings::Matrix4f_Class);
        if (s_mat4Class) {
            #define GET_MAT_FIELD(row, col) \
                s_mat4_m##row##col = env->GetFieldID(s_mat4Class, Mappings::Matrix4f_m##row##col, "F"); \
                if (env->ExceptionCheck()) { env->ExceptionClear(); s_mat4_m##row##col = nullptr; }

            GET_MAT_FIELD(0, 0); GET_MAT_FIELD(0, 1); GET_MAT_FIELD(0, 2); GET_MAT_FIELD(0, 3);
            GET_MAT_FIELD(1, 0); GET_MAT_FIELD(1, 1); GET_MAT_FIELD(1, 2); GET_MAT_FIELD(1, 3);
            GET_MAT_FIELD(2, 0); GET_MAT_FIELD(2, 1); GET_MAT_FIELD(2, 2); GET_MAT_FIELD(2, 3);
            GET_MAT_FIELD(3, 0); GET_MAT_FIELD(3, 1); GET_MAT_FIELD(3, 2); GET_MAT_FIELD(3, 3);
            
            #undef GET_MAT_FIELD
        }
        printf("[Ghost] RenderSystem matrices: %s (getProj=%p getModelView=%p)\n",
               (s_rsGetProj && s_rsGetModelView && s_mat4_m00) ? "OK" : "PARTIAL",
               s_rsGetProj, s_rsGetModelView);
    } else {
        printf("[Ghost] WARN: RenderSystem class not found\n");
    }

    printf("[Ghost] Minecraft IDs: getInstance=%p instanceField=%p player=%p level=%p\n",
           s_getInstance, s_instanceField, s_player, s_level);

    bool ok = (s_getInstance || s_instanceField) && s_player && s_level;
    printf("[Ghost] Minecraft IDs initialized: %s\n", ok ? "OK" : "PARTIAL");
    
    if (!ok) {
        if (!s_getInstance && !s_instanceField) printf("[Ghost]   FAILED: no way to get Minecraft instance\n");
        if (!s_player) printf("[Ghost]   FAILED: player field\n");
        if (!s_level)  printf("[Ghost]   FAILED: level field\n");
        JvmWrapper::dumpClassInfo(s_mcClass, "Minecraft Diagnostics", 10, 10);
    }

    return ok;
}

jobject CMinecraft::getInstance() {
    JNIEnv* env = JvmWrapper::getEnv();
    if (!env) return nullptr;

    // ── Strategy 1: Read static field directly (most reliable) ──────
    if (s_instanceField) {
        jobject mc = env->GetStaticObjectField(s_mcClass, s_instanceField);
        if (env->ExceptionCheck()) {
            static int excCount = 0;
            if (++excCount <= 3) {
                printf("[Ghost] getInstance: exception reading instance field!\n");
                env->ExceptionDescribe();
            }
            env->ExceptionClear();
        }
        if (mc) return mc;
        
        static int fieldNullCount = 0;
        if (++fieldNullCount <= 3) {
            printf("[Ghost] getInstance: static field returned null\n");
        }
    }

    // ── Strategy 2: Call getInstance() method ───────────────────────
    if (s_getInstance && s_mcClass) {
        jobject mc = env->CallStaticObjectMethod(s_mcClass, s_getInstance);
        if (env->ExceptionCheck()) {
            static int excCount = 0;
            if (++excCount <= 3) {
                printf("[Ghost] getInstance: exception calling getInstance()!\n");
                env->ExceptionDescribe();
            }
            env->ExceptionClear();
            return nullptr;
        }
        if (mc) return mc;
        
        static int methodNullCount = 0;
        if (++methodNullCount <= 3) {
            printf("[Ghost] getInstance: method returned null (no exception)\n");
        }
    }

    // ── Strategy 3: Last resort — scan all static fields via reflection ──
    static bool triedScan = false;
    if (!triedScan && s_mcClass && env) {
        triedScan = true;
        printf("[Ghost] getInstance: trying brute-force field scan...\n");
        
        jclass clsClass = env->FindClass("java/lang/Class");
        if (clsClass) {
            jmethodID getDeclaredFields = env->GetMethodID(clsClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
            if (getDeclaredFields) {
                jobjectArray fields = (jobjectArray)env->CallObjectMethod(s_mcClass, getDeclaredFields);
                if (fields && !env->ExceptionCheck()) {
                    jsize fieldCount = env->GetArrayLength(fields);
                    printf("[Ghost] getInstance: scanning %d declared fields...\n", fieldCount);
                    
                    jclass fieldClass = env->FindClass("java/lang/reflect/Field");
                    jmethodID getNameM = env->GetMethodID(fieldClass, "getName", "()Ljava/lang/String;");
                    jmethodID getModifiers = env->GetMethodID(fieldClass, "getModifiers", "()I");
                    jclass modifierClass = env->FindClass("java/lang/reflect/Modifier");
                    jmethodID isStaticM = env->GetStaticMethodID(modifierClass, "isStatic", "(I)Z");
                    
                    for (jsize i = 0; i < fieldCount; i++) {
                        jobject field = env->GetObjectArrayElement(fields, i);
                        jstring fname = (jstring)env->CallObjectMethod(field, getNameM);
                        jint mods = env->CallIntMethod(field, getModifiers);
                        bool isStatic = env->CallStaticBooleanMethod(modifierClass, isStaticM, mods);
                        
                        std::string name = JvmWrapper::jstringToString(fname);
                        
                        if (isStatic) {
                            // Try to read it as a Minecraft-typed field
                            jfieldID fid = env->GetStaticFieldID(s_mcClass, name.c_str(), 
                                "Lnet/minecraft/client/Minecraft;");
                            if (env->ExceptionCheck()) { env->ExceptionClear(); fid = nullptr; }
                            
                            if (fid) {
                                printf("[Ghost]   Static field '%s' matches Minecraft type!\n", name.c_str());
                                jobject mc = env->GetStaticObjectField(s_mcClass, fid);
                                if (mc) {
                                    printf("[Ghost]   SUCCESS: Got Minecraft instance via field '%s'!\n", name.c_str());
                                    s_instanceField = fid;
                                    env->DeleteLocalRef(field);
                                    if (fname) env->DeleteLocalRef(fname);
                                    env->DeleteLocalRef(fields);
                                    env->DeleteLocalRef(fieldClass);
                                    env->DeleteLocalRef(modifierClass);
                                    env->DeleteLocalRef(clsClass);
                                    return mc;
                                } else {
                                    printf("[Ghost]   Field '%s' has Minecraft type but value is null\n", name.c_str());
                                }
                            }
                        }
                        
                        env->DeleteLocalRef(field);
                        if (fname) env->DeleteLocalRef(fname);
                    }
                    env->DeleteLocalRef(fields);
                    env->DeleteLocalRef(fieldClass);
                    env->DeleteLocalRef(modifierClass);
                }
                if (env->ExceptionCheck()) env->ExceptionClear();
            }
            env->DeleteLocalRef(clsClass);
        }
        if (env->ExceptionCheck()) env->ExceptionClear();
    }

    return nullptr;
}

jobject CMinecraft::getPlayer() {
    if (!s_player) return nullptr;
    JNIEnv* env = JvmWrapper::getEnv();
    if (!env) return nullptr;

    jobject mc = getInstance();
    if (!mc) {
        static int warnCount = 0;
        warnCount++;
        if (warnCount <= 3 || warnCount % 1800 == 0) {
            printf("[Ghost] getPlayer: getInstance() returned null (count=%d)\n", warnCount);
        }
        return nullptr;
    }

    jobject player = env->GetObjectField(mc, s_player);
    if (JvmWrapper::checkException()) {
        env->DeleteLocalRef(mc);
        return nullptr;
    }
    
    if (!player) {
        static int nullCount = 0;
        nullCount++;
        if (nullCount <= 3 || nullCount % 1800 == 0) {
            printf("[Ghost] getPlayer: player field is null (not in world?) count=%d\n", nullCount);
        }
    }
    
    env->DeleteLocalRef(mc);
    return player;
}

jobject CMinecraft::getWorld() {
    if (!s_level) return nullptr;
    JNIEnv* env = JvmWrapper::getEnv();

    jobject mc = getInstance();
    if (!mc) return nullptr;

    jobject world = env->GetObjectField(mc, s_level);
    env->DeleteLocalRef(mc);
    return world;
}

CMinecraft::CameraData CMinecraft::getCameraData() {
    CameraData data{};
    data.valid = false;
    data.fov = 70.0f;  // default

    JNIEnv* env = JvmWrapper::getEnv();
    if (!env) return data;

    jobject mc = getInstance();
    if (!mc) return data;

    // ── Read FOV from options ────────────────────────────────────────
    if (s_options && s_optFov && s_optInstGet) {
        jobject opts = env->GetObjectField(mc, s_options);
        if (opts && !env->ExceptionCheck()) {
            jobject fovOpt = env->GetObjectField(opts, s_optFov);
            if (fovOpt && !env->ExceptionCheck()) {
                jobject fovBoxed = env->CallObjectMethod(fovOpt, s_optInstGet);
                if (fovBoxed && !env->ExceptionCheck()) {
                    // Unbox Integer → int
                    jclass intClass = env->FindClass("java/lang/Integer");
                    if (intClass) {
                        jmethodID intValue = env->GetMethodID(intClass, "intValue", "()I");
                        if (intValue) {
                            data.fov = (float)env->CallIntMethod(fovBoxed, intValue);
                        }
                        env->DeleteLocalRef(intClass);
                    }
                    env->DeleteLocalRef(fovBoxed);
                }
                if (env->ExceptionCheck()) env->ExceptionClear();
                env->DeleteLocalRef(fovOpt);
            }
            if (env->ExceptionCheck()) env->ExceptionClear();
            env->DeleteLocalRef(opts);
        }
        if (env->ExceptionCheck()) env->ExceptionClear();
    }

    // ── Read camera position from GameRenderer.getMainCamera() ──────
    jobject gr = nullptr;
    if (s_gameRenderer) {
        gr = env->GetObjectField(mc, s_gameRenderer);
        if (env->ExceptionCheck()) { env->ExceptionClear(); gr = nullptr; }
    }

    if (gr && s_getMainCamera && s_camPosition && s_camYRot && s_camXRot) {
        jobject camera = env->CallObjectMethod(gr, s_getMainCamera);
        if (camera && !env->ExceptionCheck()) {
            // Read Camera.position (Vec3 field)
            jobject pos = env->GetObjectField(camera, s_camPosition);
            if (pos && !env->ExceptionCheck()) {
                jclass v3c = env->GetObjectClass(pos);
                jfieldID fx = env->GetFieldID(v3c, "x", "D");
                jfieldID fy = env->GetFieldID(v3c, "y", "D");
                jfieldID fz = env->GetFieldID(v3c, "z", "D");
                if (fx && fy && fz) {
                    data.x = env->GetDoubleField(pos, fx);
                    data.y = env->GetDoubleField(pos, fy);
                    data.z = env->GetDoubleField(pos, fz);
                }
                if (env->ExceptionCheck()) env->ExceptionClear();
                env->DeleteLocalRef(v3c);
                env->DeleteLocalRef(pos);
            }
            if (env->ExceptionCheck()) env->ExceptionClear();

            // Read Camera.yRot, Camera.xRot (float fields)
            data.yaw   = env->GetFloatField(camera, s_camYRot);
            if (env->ExceptionCheck()) env->ExceptionClear();
            data.pitch = env->GetFloatField(camera, s_camXRot);
            if (env->ExceptionCheck()) env->ExceptionClear();

            data.valid = true;
            
            // ── Read exact dynamic FOV ────────────────────────────────────────
            if (s_getFov) {
                // getFov(Camera camera, float partialTicks, boolean useFovSetting)
                double fov = env->CallDoubleMethod(gr, s_getFov, camera, 1.0f, JNI_TRUE);
                if (!env->ExceptionCheck()) {
                    data.fov = (float)fov;
                } else {
                    env->ExceptionClear();
                }
            }
            // ------------------------------------------------------------------
            
            static int camLogCount = 0;
            if (++camLogCount <= 5 || camLogCount % 1800 == 0) {
                printf("[Ghost] CameraData: pos=(%.1f,%.1f,%.1f) yaw=%.1f pitch=%.1f fov=%.1f\n",
                       data.x, data.y, data.z, data.yaw, data.pitch, data.fov);
            }
            
            env->DeleteLocalRef(camera);
        }
        if (env->ExceptionCheck()) env->ExceptionClear();
    }

    // ── Read Window framebuffer dimensions ─────────────────────────────
    if (s_window && s_getWidth && s_getHeight) {
        jobject window = env->GetObjectField(mc, s_window);
        if (window && !env->ExceptionCheck()) {
            data.screenW = env->CallIntMethod(window, s_getWidth);
            if (env->ExceptionCheck()) env->ExceptionClear();
            data.screenH = env->CallIntMethod(window, s_getHeight);
            if (env->ExceptionCheck()) env->ExceptionClear();
            
            static int winLogCount = 0;
            if (++winLogCount <= 5 || winLogCount % 1800 == 0) {
                printf("[Ghost] Window framebuffer: %dx%d (aspect=%.3f)\n",
                       data.screenW, data.screenH,
                       data.screenH > 0 ? (double)data.screenW / data.screenH : 0.0);
            }
            env->DeleteLocalRef(window);
        }
        if (env->ExceptionCheck()) env->ExceptionClear();
    }

    // ── Read Matrices from RenderSystem ─────────────────────────────
    data.hasMatrices = false;
    if (s_rsClass && s_rsGetProj && s_rsGetModelView && s_mat4_m00) {
        jobject projMat = env->CallStaticObjectMethod(s_rsClass, s_rsGetProj);
        if (!env->ExceptionCheck() && projMat) {
            jobject viewMat = env->CallStaticObjectMethod(s_rsClass, s_rsGetModelView);
            if (!env->ExceptionCheck() && viewMat) {
                #define READ_MAT(obj, arr, r, c, idx) \
                    arr[idx] = env->GetFloatField(obj, s_mat4_m##r##c)
                
                READ_MAT(projMat, data.projMatrix, 0, 0, 0); READ_MAT(projMat, data.projMatrix, 1, 0, 1); READ_MAT(projMat, data.projMatrix, 2, 0, 2); READ_MAT(projMat, data.projMatrix, 3, 0, 3);
                READ_MAT(projMat, data.projMatrix, 0, 1, 4); READ_MAT(projMat, data.projMatrix, 1, 1, 5); READ_MAT(projMat, data.projMatrix, 2, 1, 6); READ_MAT(projMat, data.projMatrix, 3, 1, 7);
                READ_MAT(projMat, data.projMatrix, 0, 2, 8); READ_MAT(projMat, data.projMatrix, 1, 2, 9); READ_MAT(projMat, data.projMatrix, 2, 2, 10); READ_MAT(projMat, data.projMatrix, 3, 2, 11);
                READ_MAT(projMat, data.projMatrix, 0, 3, 12); READ_MAT(projMat, data.projMatrix, 1, 3, 13); READ_MAT(projMat, data.projMatrix, 2, 3, 14); READ_MAT(projMat, data.projMatrix, 3, 3, 15);

                READ_MAT(viewMat, data.viewMatrix, 0, 0, 0); READ_MAT(viewMat, data.viewMatrix, 1, 0, 1); READ_MAT(viewMat, data.viewMatrix, 2, 0, 2); READ_MAT(viewMat, data.viewMatrix, 3, 0, 3);
                READ_MAT(viewMat, data.viewMatrix, 0, 1, 4); READ_MAT(viewMat, data.viewMatrix, 1, 1, 5); READ_MAT(viewMat, data.viewMatrix, 2, 1, 6); READ_MAT(viewMat, data.viewMatrix, 3, 1, 7);
                READ_MAT(viewMat, data.viewMatrix, 0, 2, 8); READ_MAT(viewMat, data.viewMatrix, 1, 2, 9); READ_MAT(viewMat, data.viewMatrix, 2, 2, 10); READ_MAT(viewMat, data.viewMatrix, 3, 2, 11);
                READ_MAT(viewMat, data.viewMatrix, 0, 3, 12); READ_MAT(viewMat, data.viewMatrix, 1, 3, 13); READ_MAT(viewMat, data.viewMatrix, 2, 3, 14); READ_MAT(viewMat, data.viewMatrix, 3, 3, 15);
                
                #undef READ_MAT
                data.hasMatrices = true;
                env->DeleteLocalRef(viewMat);
            }
            if (env->ExceptionCheck()) env->ExceptionClear();
            env->DeleteLocalRef(projMat);
        }
        if (env->ExceptionCheck()) env->ExceptionClear();
    }


    if (gr) env->DeleteLocalRef(gr);

    env->DeleteLocalRef(mc);
    return data;
}
