#include "pch.h"
#include "modules/xray.h"
#include "sdk/minecraft.h"
#include "core/jvm_wrapper.h"
#include "render/esp_renderer.h"

XRay::XRay() : Module("X-Ray", 'X') {
    m_enabled = false;
    m_boolSettings["diamond"] = true;
    m_boolSettings["iron"]    = true;
    m_boolSettings["gold"]    = true;
    m_boolSettings["coal"]    = false;
    m_boolSettings["emerald"] = true;
    m_boolSettings["lapis"]   = false;
    m_boolSettings["redstone"]= false;
    m_boolSettings["netherite"] = true;
    m_boolSettings["spawner"] = true;

    m_floatSettings["radius"] = 30.0f;
    m_floatSettings["thickness"] = 1.0f;
}

void XRay::onEnable() {
    m_running = true;
    m_blocks.clear();
    m_searchThread = std::thread(&XRay::searchLoop, this);
}

void XRay::onDisable() {
    m_running = false;
    if (m_searchThread.joinable()) {
        m_searchThread.join();
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_blocks.clear();
}

void XRay::onRender() {
    if (!m_enabled) return;

    JNIEnv* env = JvmWrapper::getEnv();
    if (!env) return;

    auto camData = CMinecraft::getCameraData();
    if (!camData.valid) return;

    EspRenderer::setFOV(camData.fov);
    EspRenderer::setMatrices(camData.hasMatrices, camData.viewMatrix, camData.projMatrix);
    EspRenderer::updateCamera(camData.x, camData.y, camData.z, camData.yaw, camData.pitch, EspRenderer::getScreenW(), EspRenderer::getScreenH());

    std::vector<BlockData> blocksToDraw;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        blocksToDraw = m_blocks;
    }

    float thickness = m_floatSettings["thickness"];

    for (const auto& block : blocksToDraw) {
        ImU32 color = IM_COL32(255, 255, 255, 255);
        if (block.name.find("diamond") != std::string::npos) color = IM_COL32(0, 255, 255, 255);
        else if (block.name.find("gold") != std::string::npos) color = IM_COL32(255, 215, 0, 255);
        else if (block.name.find("iron") != std::string::npos) color = IM_COL32(255, 200, 150, 255);
        else if (block.name.find("emerald") != std::string::npos) color = IM_COL32(0, 255, 0, 255);
        else if (block.name.find("redstone") != std::string::npos) color = IM_COL32(255, 0, 0, 255);
        else if (block.name.find("lapis") != std::string::npos) color = IM_COL32(0, 0, 255, 255);
        else if (block.name.find("coal") != std::string::npos) color = IM_COL32(50, 50, 50, 255);
        else if (block.name.find("debris") != std::string::npos) color = IM_COL32(100, 0, 0, 255); // Ancient debris
        else if (block.name.find("spawner") != std::string::npos) color = IM_COL32(255, 100, 100, 255);

        // Draw the 3D block
        EspRenderer::draw3DBox(block.x, block.y, block.z, block.x + 1.0, block.y + 1.0, block.z + 1.0, color, thickness);
    }
}

void XRay::searchLoop() {
    JNIEnv* env = JvmWrapper::getEnv();
    if (!env) {
        printf("[Ghost] XRay thread: Failed to attach\n");
        return;
    }

    // Resolve methods once
    jclass mutablePosClass = JvmWrapper::findClass(Mappings::MutableBlockPos_Class);
    jmethodID initPos = JvmWrapper::getMethodID(mutablePosClass, Mappings::MutableBlockPos_Init, Mappings::MutableBlockPos_Init_Sig);
    jmethodID setPos = JvmWrapper::getMethodID(mutablePosClass, Mappings::MutableBlockPos_set, Mappings::MutableBlockPos_set_Sig);

    jclass levelClass = JvmWrapper::findClass(Mappings::ClientLevel_Class);
    jmethodID getBlockState = JvmWrapper::getMethodID(levelClass, Mappings::Level_getBlockState, Mappings::Level_getBlockState_Sig);

    jclass stateClass = JvmWrapper::findClass(Mappings::BlockState_Class);
    jmethodID getBlock = JvmWrapper::getMethodID(stateClass, Mappings::BlockState_getBlock, Mappings::BlockState_getBlock_Sig);

    jclass blockClass = JvmWrapper::findClass(Mappings::Block_Class);
    jmethodID getDescriptionId = JvmWrapper::getMethodID(blockClass, Mappings::Block_getDescriptionId, Mappings::Block_getDescriptionId_Sig);

    if (!initPos || !setPos || !getBlockState || !getBlock || !getDescriptionId) {
        printf("[Ghost] XRay thread: Failed to resolve methods\n");
        return;
    }

    jobject mutablePosObj = env->NewObject(mutablePosClass, initPos, 0, 0, 0);
    if (!mutablePosObj) return;
    jobject globalPos = env->NewGlobalRef(mutablePosObj);
    env->DeleteLocalRef(mutablePosObj);

    while (m_running) {
        env->PushLocalFrame(128); // Free local refs in the loop chunk

        jobject player = CMinecraft::getPlayer();
        jobject world = CMinecraft::getWorld();

        if (!player || !world) {
            env->PopLocalFrame(nullptr);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        auto camData = CMinecraft::getCameraData();
        int px = (int)camData.x;
        int py = (int)camData.y;
        int pz = (int)camData.z;

        int radius = (int)m_floatSettings["radius"];

        std::vector<BlockData> newBlocks;

        // Iterate blocks
        for (int x = px - radius; x <= px + radius && m_running; x++) {
            for (int y = py - radius; y <= py + radius && m_running; y++) {
                // Bounds check Y
                if (y < -64 || y > 320) continue; 
                for (int z = pz - radius; z <= pz + radius && m_running; z++) {
                    env->PushLocalFrame(16); // small frame per block

                    env->CallObjectMethod(globalPos, setPos, x, y, z);
                    
                    jobject state = env->CallObjectMethod(world, getBlockState, globalPos);
                    if (state && !JvmWrapper::checkException()) {
                        jobject block = env->CallObjectMethod(state, getBlock);
                        if (block && !JvmWrapper::checkException()) {
                            jstring descStr = (jstring)env->CallObjectMethod(block, getDescriptionId);
                            if (descStr && !JvmWrapper::checkException()) {
                                std::string name = JvmWrapper::jstringToString(descStr);
                                
                                // Basic filtering logic
                                if (name.find("ore") != std::string::npos || 
                                    name.find("ancient_debris") != std::string::npos ||
                                    name.find("spawner") != std::string::npos) {
                                    
                                    bool add = false;
                                    if (m_boolSettings["diamond"] && name.find("diamond") != std::string::npos) add = true;
                                    else if (m_boolSettings["iron"] && name.find("iron") != std::string::npos) add = true;
                                    else if (m_boolSettings["gold"] && name.find("gold") != std::string::npos) add = true;
                                    else if (m_boolSettings["coal"] && name.find("coal") != std::string::npos) add = true;
                                    else if (m_boolSettings["emerald"] && name.find("emerald") != std::string::npos) add = true;
                                    else if (m_boolSettings["lapis"] && name.find("lapis") != std::string::npos) add = true;
                                    else if (m_boolSettings["redstone"] && name.find("redstone") != std::string::npos) add = true;
                                    else if (m_boolSettings["netherite"] && name.find("ancient_debris") != std::string::npos) add = true;
                                    else if (m_boolSettings["spawner"] && name.find("spawner") != std::string::npos) add = true;

                                    if (add) {
                                        // Format the name a bit cleaner
                                        size_t lastDot = name.find_last_of('.');
                                        if (lastDot != std::string::npos) {
                                            name = name.substr(lastDot + 1);
                                        }
                                        newBlocks.push_back({x, y, z, name});
                                    }
                                }
                            }
                        }
                    }
                    env->PopLocalFrame(nullptr);
                }
            }
            // Yield briefly per X-slice so we don't monopolize CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        // Swap the buffer if we completed a scan
        if (m_running) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_blocks = std::move(newBlocks);
        }

        env->PopLocalFrame(nullptr);
        
        // Wait before scanning again (1s)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    env->DeleteGlobalRef(globalPos);
}
