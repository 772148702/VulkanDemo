#include "DVKModel.h"
#include "FileManager.h"
#include "Math/Matrix4x4.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"


namespace vk_dem 
{
    void SimplifyTexturePath(std::string& path)
    {
        const size_t lastSlashIdx = path.find_last_of("\\/");
        if (std::string::npos != lastSlashIdx)
        {
            path.erase(0, lastSlashIdx + 1);
        }

        const size_t periodIdx = path.rfind('.');
        if (std::string::npos != periodIdx)
        {
            path.erase(periodIdx);
        }
    }

    void FillMaterialTextures(aiMaterial* aiMaterial, DVKMaterialInfo& material)
    {
        
    }

}