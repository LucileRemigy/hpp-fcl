/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2019, CNRS - LAAS
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Open Source Robotics Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <hpp/fcl/mesh_loader/assimp.h>

#ifdef HPP_FCL_USE_ASSIMP_UNIFIED_HEADER_NAMES
  #include <assimp/DefaultLogger.hpp>
  #include <assimp/IOStream.hpp>
  #include <assimp/IOSystem.hpp>
  #include <assimp/Importer.hpp>
  #include <assimp/postprocess.h>
  #include <assimp/scene.h>
#else
  #include <assimp/DefaultLogger.h>
  #include <assimp/assimp.hpp>
  #include <assimp/IOStream.h>
  #include <assimp/IOSystem.h>
  #include <assimp/aiPostProcess.h>
  #include <assimp/aiScene.h>
#endif

namespace hpp
{
namespace fcl
{
  
namespace internal
{

Loader::Loader () : importer (new Assimp::Importer())
{
  // set list of ignored parameters (parameters used for rendering)
  importer->SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
      aiComponent_TANGENTS_AND_BITANGENTS|
      aiComponent_COLORS |
      aiComponent_BONEWEIGHTS |
      aiComponent_ANIMATIONS |
      aiComponent_LIGHTS |
      aiComponent_CAMERAS|
      aiComponent_TEXTURES |
      aiComponent_TEXCOORDS |
      aiComponent_MATERIALS |
      aiComponent_NORMALS
      );

}

Loader::~Loader ()
{
 if (importer) delete importer;
}

void Loader::load (const std::string & resource_path)
{
  scene = importer->ReadFile(resource_path.c_str(),
      aiProcess_SortByPType |
      aiProcess_Triangulate |
      aiProcess_RemoveComponent |
      aiProcess_ImproveCacheLocality |
      // TODO: I (Joseph Mirabel) have no idea whether degenerated triangles are
      // properly handled. Enabling aiProcess_FindDegenerates would throw an
      // exception when that happens. Is it too conservative ?
      // aiProcess_FindDegenerates |
      aiProcess_JoinIdenticalVertices
      );

  if (!scene)
  {
    const std::string exception_message (std::string ("Could not load resource ") + resource_path + std::string("\n") +
                                         importer->GetErrorString () + std::string("\n") +
                                         "Hint: the mesh directory may be wrong.");
    throw std::invalid_argument(exception_message);
  }

  if (!scene->HasMeshes())
    throw std::invalid_argument (std::string ("No meshes found in file ")+resource_path);
}

/**
 * @brief      Recursive procedure for building a mesh
 *
 * @param[in]  scale           Scale to apply when reading the ressource
 * @param[in]  scene           Pointer to the assimp scene
 * @param[in]  node            Current node of the scene
 * @param[in]  vertices_offset Current number of vertices in the model
 * @param      tv              Triangles and Vertices of the mesh submodels
 */
unsigned recurseBuildMesh (
    const fcl::Vec3f & scale,
    const aiScene* scene,
    const aiNode* node,
    unsigned vertices_offset,
    TriangleAndVertices & tv)
{
  if (!node) return 0;
  
  aiMatrix4x4 transform = node->mTransformation;
  aiNode *pnode = node->mParent;
  while (pnode)
  {
    // Don't convert to y-up orientation, which is what the root node in
    // Assimp does
    if (pnode->mParent != NULL)
    {
      transform = pnode->mTransformation * transform;
    }
    pnode = pnode->mParent;
  }
  
  unsigned nbVertices = 0;
  for (uint32_t i = 0; i < node->mNumMeshes; i++)
  {
    aiMesh* input_mesh = scene->mMeshes[node->mMeshes[i]];
    
    // Add the vertices
    for (uint32_t j = 0; j < input_mesh->mNumVertices; j++)
    {
      aiVector3D p = input_mesh->mVertices[j];
      p *= transform;
      tv.vertices_.push_back (fcl::Vec3f (p.x * scale[0],
                                          p.y * scale[1],
                                          p.z * scale[2]));
    }
    
    // add the indices
    for (uint32_t j = 0; j < input_mesh->mNumFaces; j++)
    {
      aiFace& face = input_mesh->mFaces[j];
      if (face.mNumIndices != 3) {
        std::stringstream ss;
#ifdef HPP_FCL_USE_ASSIMP_UNIFIED_HEADER_NAMES
        ss << "Mesh " << input_mesh->mName.C_Str() << " has a face with "
           << face.mNumIndices << " vertices. This is not supported\n";
        ss << "Node name is: " << node->mName.C_Str() << "\n";
#endif
        ss << "Mesh index: " << i << "\n";
        ss << "Face index: " << j << "\n";
        throw std::invalid_argument (ss.str());
      }
      tv.triangles_.push_back (fcl::Triangle(vertices_offset + face.mIndices[0],
                                             vertices_offset + face.mIndices[1],
                                             vertices_offset + face.mIndices[2]));
    }

    nbVertices += input_mesh->mNumVertices;
  }
  
  for (uint32_t i=0; i < node->mNumChildren; ++i)
  {
    nbVertices += recurseBuildMesh(scale, scene, node->mChildren[i], nbVertices, tv);
  }

  return nbVertices;
}

void buildMesh (const fcl::Vec3f & scale,
                    const aiScene* scene,
                    unsigned vertices_offset,
                    TriangleAndVertices & tv)
{
  recurseBuildMesh (scale, scene, scene->mRootNode, vertices_offset, tv);
}

} // namespace internal
} // namespace fcl
} // namespace hpp
