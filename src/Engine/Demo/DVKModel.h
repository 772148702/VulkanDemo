#pragma once
#include "../Engine.h"
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

        DVKBoundingBox()  
          : min(MAX_flt, MAX_flt, MAX_flt)
            , max(MIN_flt, MIN_flt, MIN_flt)
        {

        }

        DVKBoundingBox(const Vector3& inMin,const Vector3& inMax)
        :min(inMax),max(inMax)
        {

        }

        void UpdateCorners()
        {
            corners[0].Set(min.x, min.y, min.z);
            corners[1].Set(max.x, min.y, min.z);
            corners[2].Set(min.x, max.y, min.z);
            corners[3].Set(max.x, max.y, min.z);

            corners[4].Set(min.x, min.y, max.z);
            corners[5].Set(max.x, min.y, max.z);
            corners[6].Set(min.x, max.y, max.z);
            corners[7].Set(max.x, max.y, max.z);
        }


    };
}