#include "MaterialLibrary.h"

#include "render_target.h"

#include "../shading/material.h"
#include "../resources/resources.h"

#include "../../utility/logging/log.h"
#include "../../utility/string_id.h"

namespace Cell
{
    MaterialLibrary::MaterialLibrary(RenderTarget* gBuffer)
    {
        generateDefaultMaterials();
        generateInternalMaterials(gBuffer);
    }
   
    MaterialLibrary::~MaterialLibrary()
    {
        for (auto it = m_DefaultMaterials.begin(); it != m_DefaultMaterials.end(); ++it)
        {
            delete it->second;
        }
        for (unsigned int i = 0; i < m_Materials.size(); ++i)
        {
            delete m_Materials[i];
        }
        delete debugLightMaterial;

        delete defaultBlitMaterial;
    }
   
    Material* MaterialLibrary::CreateMaterial(std::string base)
    {
        auto found = m_DefaultMaterials.find(SID(base));
        if (found != m_DefaultMaterials.end())
        {
            Material copy = found->second->Copy();
            Material* mat = new Material(copy);
            m_Materials.push_back(mat); // TODO(Joey): a bit ugly for now, come up with a bettermemory management scheme for materials
            return mat;
        }
        else
        {
            Log::Message("Material of template: " + base + " requested, but template did not exist.", LOG_ERROR);
            return nullptr;
        }
    }
   
    Material* MaterialLibrary::CreateCustomMaterial(Shader* shader)
    {
        Material* mat = new Material(shader);
        mat->Type = MATERIAL_CUSTOM;
        m_Materials.push_back(mat);
        return mat;
    }
   
    Material* MaterialLibrary::CreatePostProcessingMaterial(Shader* shader)
    {
        Material* mat = new Material(shader);
        mat->Type = MATERIAL_POST_PROCESS;
        m_Materials.push_back(mat);
        return mat;
    }
    
    void MaterialLibrary::generateDefaultMaterials()
    {
        // default render material (deferred path)
        Shader* defaultShader = Resources::LoadShader("default", "D:/engle/dragon3/dragon3/build/shaders/deferred/g_buffer.vs", "D:/engle/dragon3/dragon3/build/shaders/deferred/g_buffer.fs");
        Material* defaultMat = new Material(defaultShader);
        defaultMat->Type = MATERIAL_DEFAULT;
        defaultMat->SetTexture("TexAlbedo", Resources::LoadTexture("default albedo", "D:/engle/dragon3/dragon3/build/textures/checkerboard.png", GL_TEXTURE_2D, GL_RGB), 3);
        defaultMat->SetTexture("TexNormal", Resources::LoadTexture("default normal", "D:/engle/dragon3/dragon3/build/textures/norm.png"), 4);
        defaultMat->SetTexture("TexMetallic", Resources::LoadTexture("default metallic", "D:/engle/dragon3/dragon3/build/textures/black.png"), 5);
        defaultMat->SetTexture("TexRoughness", Resources::LoadTexture("default roughness", "D:/engle/dragon3/dragon3/build/textures/checkerboard.png"), 6);
        m_DefaultMaterials[SID("default")] = defaultMat;
        // glass material
        Shader* glassShader = Resources::LoadShader("glass", "D:/engle/dragon3/dragon3/build/shaders/forward_render.vs", "D:/engle/dragon3/dragon3/build/shaders/forward_render.fs", { "ALPHA_BLEND" });
        glassShader->Use();
        glassShader->SetInt("lightShadowMap1", 10);
        glassShader->SetInt("lightShadowMap2", 10);
        glassShader->SetInt("lightShadowMap3", 10);
        glassShader->SetInt("lightShadowMap4", 10);
        Material* glassMat = new Material(glassShader);
        glassMat->Type = MATERIAL_CUSTOM; // this material can't fit in the deferred rendering pipeline (due to transparency sorting).
        glassMat->SetTexture("TexAlbedo", Cell::Resources::LoadTexture("glass albedo", "D:/engle/dragon3/dragon3/build/textures/glass.png", GL_TEXTURE_2D, GL_RGBA), 0);
        glassMat->SetTexture("TexNormal", Cell::Resources::LoadTexture("glass normal", "D:/engle/dragon3/dragon3/build/textures/pbr/plastic/normal.png"), 1);
        glassMat->SetTexture("TexMetallic", Cell::Resources::LoadTexture("glass metallic", "D:/engle/dragon3/dragon3/build/textures/pbr/plastic/metallic.png"), 2);
        glassMat->SetTexture("TexRoughness", Cell::Resources::LoadTexture("glass roughness", "D:/engle/dragon3/dragon3/build/textures/pbr/plastic/roughness.png"), 3);
        glassMat->SetTexture("TexAO", Cell::Resources::LoadTexture("glass ao", "D:/engle/dragon3/dragon3/build/textures/pbr/plastic/ao.png"), 4);
        glassMat->Blend = true;
        m_DefaultMaterials[SID("glass")] = glassMat;
        // alpha blend material
        Shader* alphaBlendShader = Resources::LoadShader("alpha blend", "D:/engle/dragon3/dragon3/build/shaders/forward_render.vs", "D:/engle/dragon3/dragon3/build/shaders/forward_render.fs", { "ALPHA_BLEND" });
        alphaBlendShader->Use();
        alphaBlendShader->SetInt("lightShadowMap1", 10);
        alphaBlendShader->SetInt("lightShadowMap2", 10);
        alphaBlendShader->SetInt("lightShadowMap3", 10);
        alphaBlendShader->SetInt("lightShadowMap4", 10);
        Material* alphaBlendMaterial = new Material(alphaBlendShader);
        alphaBlendMaterial->Type = MATERIAL_CUSTOM;
        alphaBlendMaterial->Blend = true;
        m_DefaultMaterials[SID("alpha blend")] = alphaBlendMaterial;
        // alpha cutout material
        Shader* alphaDiscardShader = Resources::LoadShader("alpha discard", "D:/engle/dragon3/dragon3/build/shaders/forward_render.vs", "D:/engle/dragon3/dragon3/build/shaders/forward_render.fs", { "ALPHA_DISCARD" });
        alphaDiscardShader->Use();
        alphaDiscardShader->SetInt("lightShadowMap1", 10);
        alphaDiscardShader->SetInt("lightShadowMap2", 10);
        alphaDiscardShader->SetInt("lightShadowMap3", 10);
        alphaDiscardShader->SetInt("lightShadowMap4", 10);
        Material* alphaDiscardMaterial = new Material(alphaDiscardShader);
        alphaDiscardMaterial->Type = MATERIAL_CUSTOM;
        alphaDiscardMaterial->Cull = false;
        m_DefaultMaterials[SID("alpha discard")] = alphaDiscardMaterial;
    }
  
    void MaterialLibrary::generateInternalMaterials(RenderTarget *gBuffer)
    {
        // post-processing
        Shader* defaultBlitShader = Cell::Resources::LoadShader("blit", "D:/engle/dragon3/dragon3/build/shaders/screen_quad.vs", "D:/engle/dragon3/dragon3/build/shaders/default_blit.fs");
        defaultBlitMaterial = new Material(defaultBlitShader);
        
        // deferred
        deferredAmbientShader     = Cell::Resources::LoadShader("deferred ambient", "D:/engle/dragon3/dragon3/build/shaders/deferred/screen_ambient.vs", "D:/engle/dragon3/dragon3/build/shaders/deferred/ambient.fs");
        deferredIrradianceShader  = Cell::Resources::LoadShader("deferred irradiance", "D:/engle/dragon3/dragon3/build/shaders/deferred/ambient_irradience.vs", "D:/engle/dragon3/dragon3/build/shaders/deferred/ambient_irradience.fs");
        deferredDirectionalShader = Cell::Resources::LoadShader("deferred directional", "D:/engle/dragon3/dragon3/build/shaders/deferred/screen_directional.vs", "D:/engle/dragon3/dragon3/build/shaders/deferred/directional.fs");
        deferredPointShader       = Cell::Resources::LoadShader("deferred point", "D:/engle/dragon3/dragon3/build/shaders/deferred/point.vs", "D:/engle/dragon3/dragon3/build/shaders/deferred/point.fs");

        deferredAmbientShader->Use();
        deferredAmbientShader->SetInt("gPositionMetallic", 0);
        deferredAmbientShader->SetInt("gNormalRoughness", 1);
        deferredAmbientShader->SetInt("gAlbedoAO", 2);
        deferredAmbientShader->SetInt("envIrradiance", 3);
        deferredAmbientShader->SetInt("envPrefilter", 4);
        deferredAmbientShader->SetInt("BRDFLUT", 5);
        deferredAmbientShader->SetInt("TexSSAO", 6);
        deferredIrradianceShader->Use();
        deferredIrradianceShader->SetInt("gPositionMetallic", 0);
        deferredIrradianceShader->SetInt("gNormalRoughness", 1);
        deferredIrradianceShader->SetInt("gAlbedoAO", 2);
        deferredIrradianceShader->SetInt("envIrradiance", 3);
        deferredIrradianceShader->SetInt("envPrefilter", 4);
        deferredIrradianceShader->SetInt("BRDFLUT", 5);
        deferredIrradianceShader->SetInt("TexSSAO", 6);
        deferredDirectionalShader->Use();
        deferredDirectionalShader->SetInt("gPositionMetallic", 0);
        deferredDirectionalShader->SetInt("gNormalRoughness", 1);
        deferredDirectionalShader->SetInt("gAlbedoAO", 2);
        deferredDirectionalShader->SetInt("lightShadowMap", 3);
        deferredPointShader->Use();
        deferredPointShader->SetInt("gPositionMetallic", 0);
        deferredPointShader->SetInt("gNormalRoughness", 1);
        deferredPointShader->SetInt("gAlbedoAO", 2);

        // shadows
        dirShadowShader = Cell::Resources::LoadShader("shadow directional", "D:/engle/dragon3/dragon3/build/shaders/shadow_cast.vs", "D:/engle/dragon3/dragon3/build/shaders/shadow_cast.fs");

        // debug
        Shader *debugLightShader = Resources::LoadShader("debug light", "D:/engle/dragon3/dragon3/build/shaders/light.vs", "D:/engle/dragon3/dragon3/build/shaders/light.fs");
        debugLightMaterial = new Material(debugLightShader);

		//man
		ManShader = Cell::Resources::LoadShader("man", "D:/engle/dragon3/dragon3/build/shaders/custom/man.vs", "D:/engle/dragon3/dragon3/build/shaders/custom/man.fs");
    }
}