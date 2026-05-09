#pragma once

/*
 * Mapping Table — Minecraft 1.21.1 (Forge / NeoForge)
 *
 * Forge 1.21.1 uses Mojang (deobfuscated) names at runtime.
 * All class paths use JNI '/' separator format.
 * Method and field names are NOT obfuscated in the Forge runtime.
 */

namespace Mappings {

    // ─── Classes (JNI path format — Mojang deobfuscated) ────────────────
    inline constexpr const char* Minecraft_Class    = "net/minecraft/client/Minecraft";
    inline constexpr const char* Entity_Class       = "net/minecraft/world/entity/Entity";
    inline constexpr const char* LivingEntity_Class = "net/minecraft/world/entity/LivingEntity";
    inline constexpr const char* LocalPlayer_Class  = "net/minecraft/client/player/LocalPlayer";
    inline constexpr const char* ClientLevel_Class  = "net/minecraft/client/multiplayer/ClientLevel";
    inline constexpr const char* Vec3_Class         = "net/minecraft/world/phys/Vec3";
    inline constexpr const char* AABB_Class         = "net/minecraft/world/phys/AABB";
    inline constexpr const char* Player_Class       = "net/minecraft/world/entity/player/Player";
    inline constexpr const char* List_Class         = "java/util/List";
    inline constexpr const char* GameRenderer_Class = "net/minecraft/client/renderer/GameRenderer";
    inline constexpr const char* Connection_Class   = "net/minecraft/network/Connection";
    inline constexpr const char* RenderSystem_Class = "com/mojang/blaze3d/systems/RenderSystem";
    inline constexpr const char* Matrix4f_Class     = "org/joml/Matrix4f";
    inline constexpr const char* GameProfile_Class  = "com/mojang/authlib/GameProfile";
    inline constexpr const char* Component_Class    = "net/minecraft/network/chat/Component";
    inline constexpr const char* AbstractClientPlayer_Class = "net/minecraft/client/player/AbstractClientPlayer";
    inline constexpr const char* ItemEntity_Class   = "net/minecraft/world/entity/item/ItemEntity";
    inline constexpr const char* ArmorStand_Class   = "net/minecraft/world/entity/decoration/ArmorStand";
    inline constexpr const char* MinecartChest_Class= "net/minecraft/world/entity/vehicle/MinecartChest";
    inline constexpr const char* Camera_Class       = "net/minecraft/client/Camera";
    inline constexpr const char* Options_Class      = "net/minecraft/client/Options";
    inline constexpr const char* Level_Class        = "net/minecraft/world/level/Level";

    // ─── Minecraft Methods ─────────────────────────────────────────────
    inline constexpr const char* MC_getInstance     = "getInstance";
    inline constexpr const char* MC_getInstance_Sig = "()Lnet/minecraft/client/Minecraft;";

    // ─── Minecraft Fields ──────────────────────────────────────────────
    inline constexpr const char* MC_player          = "player";
    inline constexpr const char* MC_player_Sig      = "Lnet/minecraft/client/player/LocalPlayer;";
    inline constexpr const char* MC_level           = "level";
    inline constexpr const char* MC_level_Sig       = "Lnet/minecraft/client/multiplayer/ClientLevel;";
    inline constexpr const char* MC_gameRenderer    = "gameRenderer";
    inline constexpr const char* MC_gameRenderer_Sig = "Lnet/minecraft/client/renderer/GameRenderer;";
    inline constexpr const char* MC_options          = "options";
    inline constexpr const char* MC_options_Sig      = "Lnet/minecraft/client/Options;";

    // ─── GameRenderer Methods ────────────────────────────────────────
    inline constexpr const char* GR_getMainCamera     = "getMainCamera";
    inline constexpr const char* GR_getMainCamera_Sig = "()Lnet/minecraft/client/Camera;";
    inline constexpr const char* GR_getFov            = "getFov";
    inline constexpr const char* GR_getFov_Sig        = "(Lnet/minecraft/client/Camera;FZ)D";


    // ─── Camera Methods ──────────────────────────────────────────────
    inline constexpr const char* Camera_getPosition     = "getPosition";
    inline constexpr const char* Camera_getPosition_Sig = "()Lnet/minecraft/world/phys/Vec3;";
    inline constexpr const char* Camera_getXRot         = "getXRot";
    inline constexpr const char* Camera_getXRot_Sig     = "()F";
    inline constexpr const char* Camera_getYRot         = "getYRot";
    inline constexpr const char* Camera_getYRot_Sig     = "()F";

    // ─── Options Fields ──────────────────────────────────────────────
    inline constexpr const char* Options_fov            = "fov";
    // In 1.21.1, fov is an OptionInstance<Integer> — we access via fov().get()

    // ─── Entity Methods ────────────────────────────────────────────────
    inline constexpr const char* Entity_getX        = "getX";
    inline constexpr const char* Entity_getX_Sig    = "()D";
    inline constexpr const char* Entity_getY        = "getY";
    inline constexpr const char* Entity_getY_Sig    = "()D";
    inline constexpr const char* Entity_getZ        = "getZ";
    inline constexpr const char* Entity_getZ_Sig    = "()D";

    inline constexpr const char* Entity_isAlive     = "isAlive";
    inline constexpr const char* Entity_isAlive_Sig = "()Z";
    inline constexpr const char* Entity_getId       = "getId";
    inline constexpr const char* Entity_getId_Sig   = "()I";

    inline constexpr const char* Entity_getBoundingBox     = "getBoundingBox";
    inline constexpr const char* Entity_getBoundingBox_Sig = "()Lnet/minecraft/world/phys/AABB;";

    inline constexpr const char* Entity_getDeltaMovement     = "getDeltaMovement";
    inline constexpr const char* Entity_getDeltaMovement_Sig = "()Lnet/minecraft/world/phys/Vec3;";

    inline constexpr const char* Entity_setDeltaMovement     = "setDeltaMovement";
    inline constexpr const char* Entity_setDeltaMovement_Sig = "(Lnet/minecraft/world/phys/Vec3;)V";

    inline constexpr const char* Entity_getYRot     = "getYRot";
    inline constexpr const char* Entity_getYRot_Sig = "()F";
    inline constexpr const char* Entity_getXRot     = "getXRot";
    inline constexpr const char* Entity_getXRot_Sig = "()F";

    inline constexpr const char* Entity_setYRot     = "setYRot";
    inline constexpr const char* Entity_setYRot_Sig = "(F)V";
    inline constexpr const char* Entity_setXRot     = "setXRot";
    inline constexpr const char* Entity_setXRot_Sig = "(F)V";

    inline constexpr const char* Entity_getName     = "getName";
    inline constexpr const char* Entity_getName_Sig = "()Lnet/minecraft/network/chat/Component;";
    inline constexpr const char* Entity_getScoreboardName     = "getScoreboardName";
    inline constexpr const char* Entity_getScoreboardName_Sig = "()Ljava/lang/String;";
    inline constexpr const char* Entity_setSharedFlag     = "setSharedFlag";
    inline constexpr const char* Entity_setSharedFlag_Sig = "(IZ)V";
    inline constexpr const char* Entity_getSharedFlag     = "getSharedFlag";
    inline constexpr const char* Entity_getSharedFlag_Sig = "(I)Z";

    // ─── LivingEntity Methods ──────────────────────────────────────────
    inline constexpr const char* LivingEntity_getHealth     = "getHealth";
    inline constexpr const char* LivingEntity_getHealth_Sig = "()F";
    inline constexpr const char* LivingEntity_getMaxHealth     = "getMaxHealth";
    inline constexpr const char* LivingEntity_getMaxHealth_Sig = "()F";

    // ─── Player Methods ────────────────────────────────────────────────
    inline constexpr const char* Player_getGameProfile     = "getGameProfile";
    inline constexpr const char* Player_getGameProfile_Sig = "()Lcom/mojang/authlib/GameProfile;";

    // GameProfile.getName() — external library, never obfuscated
    inline constexpr const char* GameProfile_getName       = "getName";
    inline constexpr const char* GameProfile_getName_Sig   = "()Ljava/lang/String;";

    // ─── ClientLevel / Level ──────────────────────────────────────────
    inline constexpr const char* Level_players      = "players";
    inline constexpr const char* Level_players_Sig  = "Ljava/util/List;";
    
    inline constexpr const char* Level_getBlockState = "getBlockState";
    inline constexpr const char* Level_getBlockState_Sig = "(Lnet/minecraft/core/BlockPos;)Lnet/minecraft/world/level/block/state/BlockState;";

    // ─── Block & BlockState ──────────────────────────────────────────
    inline constexpr const char* MutableBlockPos_Class = "net/minecraft/core/BlockPos$MutableBlockPos";
    inline constexpr const char* MutableBlockPos_Init = "<init>";
    inline constexpr const char* MutableBlockPos_Init_Sig = "(III)V";
    inline constexpr const char* MutableBlockPos_set = "set";
    inline constexpr const char* MutableBlockPos_set_Sig = "(III)Lnet/minecraft/core/BlockPos$MutableBlockPos;";
    
    inline constexpr const char* BlockState_Class = "net/minecraft/world/level/block/state/BlockState";
    inline constexpr const char* BlockState_getBlock = "getBlock";
    inline constexpr const char* BlockState_getBlock_Sig = "()Lnet/minecraft/world/level/block/Block;";

    // ─── Entity Physics ────────────────────────────────────────────────
    inline constexpr const char* Entity_isInWater     = "isInWater";
    inline constexpr const char* Entity_isInWater_Sig = "()Z";

    inline constexpr const char* Entity_onGround     = "onGround";
    inline constexpr const char* Entity_onGround_Sig = "Z";
    inline constexpr const char* Entity_horizontalCollision = "horizontalCollision";
    inline constexpr const char* Entity_horizontalCollision_Sig = "Z";
    inline constexpr const char* Entity_wasTouchingWater = "wasTouchingWater";
    inline constexpr const char* Entity_wasTouchingWater_Sig = "Z";

    inline constexpr const char* Entity_isShiftKeyDown     = "isShiftKeyDown";
    inline constexpr const char* Entity_isShiftKeyDown_Sig = "()Z";

    inline constexpr const char* LivingEntity_discardFriction = "discardFriction";
    inline constexpr const char* LivingEntity_discardFriction_Sig = "Z";
    inline constexpr const char* LivingEntity_jumping = "jumping";
    inline constexpr const char* LivingEntity_jumping_Sig = "Z";

    inline constexpr const char* Block_Class = "net/minecraft/world/level/block/Block";
    inline constexpr const char* Block_getName = "getName";
    inline constexpr const char* Block_getName_Sig = "()Lnet/minecraft/network/chat/Component;";
    inline constexpr const char* Block_getDescriptionId = "getDescriptionId";
    inline constexpr const char* Block_getDescriptionId_Sig = "()Ljava/lang/String;";

    // ─── java.util.List Methods (JDK — never obfuscated) ───────────────
    inline constexpr const char* List_size          = "size";
    inline constexpr const char* List_size_Sig      = "()I";
    inline constexpr const char* List_get           = "get";
    inline constexpr const char* List_get_Sig       = "(I)Ljava/lang/Object;";

    // ─── Vec3 Fields ───────────────────────────────────────────────────
    inline constexpr const char* Vec3_x     = "x";
    inline constexpr const char* Vec3_y     = "y";
    inline constexpr const char* Vec3_z     = "z";
    inline constexpr const char* Vec3_D_Sig = "D";

    // ─── AABB Fields ───────────────────────────────────────────────────
    inline constexpr const char* AABB_minX  = "minX";
    inline constexpr const char* AABB_minY  = "minY";
    inline constexpr const char* AABB_minZ  = "minZ";
    inline constexpr const char* AABB_maxX  = "maxX";
    inline constexpr const char* AABB_maxY  = "maxY";
    inline constexpr const char* AABB_maxZ  = "maxZ";
    inline constexpr const char* AABB_D_Sig = "D";

    // ─── AABB Methods ──────────────────────────────────────────────────
    inline constexpr const char* AABB_inflate     = "inflate";
    inline constexpr const char* AABB_inflate_Sig = "(DDD)Lnet/minecraft/world/phys/AABB;";

    // ─── RenderSystem Methods ──────────────────────────────────────────
    inline constexpr const char* RS_getModelViewMatrix     = "getModelViewMatrix";
    inline constexpr const char* RS_getModelViewMatrix_Sig = "()Lorg/joml/Matrix4f;";
    inline constexpr const char* RS_getProjectionMatrix     = "getProjectionMatrix";
    inline constexpr const char* RS_getProjectionMatrix_Sig = "()Lorg/joml/Matrix4f;";
    inline constexpr const char* RS_getProjMatrixBuffer     = "getProjectionMatrixBuffer";
    inline constexpr const char* RS_getProjMatrixBuffer_Sig = "()Lcom/mojang/blaze3d/buffers/GpuBufferSlice;";

    // ─── GameRenderer Methods (1.21.1 Fallback) ────────────────────────
    inline constexpr const char* GR_getProjectionMatrix     = "getProjectionMatrix";
    inline constexpr const char* GR_getProjectionMatrix_Sig = "(D)Lorg/joml/Matrix4f;";

    // ─── Matrix4f Fields (JOML — never obfuscated) ─────────────────────
    inline constexpr const char* Matrix4f_m00 = "m00";
    inline constexpr const char* Matrix4f_m01 = "m01";
    inline constexpr const char* Matrix4f_m02 = "m02";
    inline constexpr const char* Matrix4f_m03 = "m03";
    inline constexpr const char* Matrix4f_m10 = "m10";
    inline constexpr const char* Matrix4f_m11 = "m11";
    inline constexpr const char* Matrix4f_m12 = "m12";
    inline constexpr const char* Matrix4f_m13 = "m13";
    inline constexpr const char* Matrix4f_m20 = "m20";
    inline constexpr const char* Matrix4f_m21 = "m21";
    inline constexpr const char* Matrix4f_m22 = "m22";
    inline constexpr const char* Matrix4f_m23 = "m23";
    inline constexpr const char* Matrix4f_m30 = "m30";
    inline constexpr const char* Matrix4f_m31 = "m31";
    inline constexpr const char* Matrix4f_m32 = "m32";
    inline constexpr const char* Matrix4f_m33 = "m33";

    // ─── Network / Packet Classes ────────────────────────────────────
    inline constexpr const char* ClientPacketListener_Class = "net/minecraft/client/multiplayer/ClientPacketListener";
    inline constexpr const char* ServerboundMovePlayerPacket_Pos_Class = "net/minecraft/network/protocol/game/ServerboundMovePlayerPacket$Pos";
    inline constexpr const char* ServerboundInteractPacket_Class = "net/minecraft/network/protocol/game/ServerboundInteractPacket";

    // Minecraft.getConnection() → ClientPacketListener
    inline constexpr const char* MC_getConnection = "getConnection";
    inline constexpr const char* MC_getConnection_Sig = "()Lnet/minecraft/client/multiplayer/ClientPacketListener;";

    // Connection.send(Packet) — Connection is a field of ClientCommonPacketListenerImpl
    inline constexpr const char* Connection_send = "send";
    inline constexpr const char* Connection_send_Sig = "(Lnet/minecraft/network/protocol/Packet;)V";
    inline constexpr const char* ClientCommonPacketListenerImpl_Class = "net/minecraft/client/multiplayer/ClientCommonPacketListenerImpl";
    inline constexpr const char* CCPLI_connection = "connection";
    inline constexpr const char* CCPLI_connection_Sig = "Lnet/minecraft/network/Connection;";

    // ServerboundMovePlayerPacket.Pos constructor: (double x, double y, double z, float yaw, float pitch, boolean onGround)
    inline constexpr const char* MovePacketPos_Init = "<init>";
    inline constexpr const char* MovePacketPos_Init_Sig = "(DDDFFZ)V";

    // ServerboundInteractPacket.createAttackPacket(Entity, boolean sneaking)
    inline constexpr const char* InteractPacket_createAttackPacket = "createAttackPacket";
    inline constexpr const char* InteractPacket_createAttackPacket_Sig = "(Lnet/minecraft/world/entity/Entity;Z)Lnet/minecraft/network/protocol/game/ServerboundInteractPacket;";

    // Minecraft.hitResult field (HitResult) — for detecting what entity the player is targeting
    inline constexpr const char* MC_hitResult = "hitResult";
    inline constexpr const char* MC_hitResult_Sig = "Lnet/minecraft/world/phys/HitResult;";

    // HitResult.getType() — returns HitResult.Type enum
    inline constexpr const char* HitResult_Class = "net/minecraft/world/phys/HitResult";
    inline constexpr const char* HitResult_getType = "getType";
    inline constexpr const char* HitResult_getType_Sig = "()Lnet/minecraft/world/phys/HitResult$Type;";

    // EntityHitResult.getEntity()
    inline constexpr const char* EntityHitResult_Class = "net/minecraft/world/phys/EntityHitResult";
    inline constexpr const char* EntityHitResult_getEntity = "getEntity";
    inline constexpr const char* EntityHitResult_getEntity_Sig = "()Lnet/minecraft/world/entity/Entity;";

    // Player.attackStrengthTicker (int field — 0 = full cooldown)
    inline constexpr const char* Player_attackStrengthTicker = "attackStrengthTicker";
    inline constexpr const char* Player_attackStrengthTicker_Sig = "I";

    // Player.swing(InteractionHand)
    inline constexpr const char* Player_swing = "swing";
    inline constexpr const char* Player_swing_Sig = "(Lnet/minecraft/world/InteractionHand;)V";

    // InteractionHand.MAIN_HAND enum constant
    inline constexpr const char* InteractionHand_Class = "net/minecraft/world/InteractionHand";
    inline constexpr const char* InteractionHand_MAIN_HAND = "MAIN_HAND";
    inline constexpr const char* InteractionHand_MAIN_HAND_Sig = "Lnet/minecraft/world/InteractionHand;";
}
