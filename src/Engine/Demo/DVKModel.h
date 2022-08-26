#pragma once
#include "Engine.h"
#include "DVKCommand.h"
#include "DVKBuffer.h"
#include "DVKIndexBuffer.h"
#include "DVKVertexBuffer.h"

#include "Common/Common.h"
#include "Math/Math.h"
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"
#include "Math/Quat.h"

#include "Vulkan/VulkanCommon.h"

#include <string>
#include <cstring>
#include <vector>
#include <memory>
#include <unordered_map>

struct aiMesh;
struct aiScene;
struct aiNode;

namespace vk_demo
{
    struct DVKNode;

    struct DVKBoundingBox
    {
        Vector3 min;
        Vector3 max;
        Vector3 corners[8];
    };
}