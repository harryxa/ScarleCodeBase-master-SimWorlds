//--------------------------------------------------------------------------------------
// File: ModelLoadSDKMESH.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Model.h"

#include "Effects.h"
#include "VertexTypes.h"

#include "DirectXHelpers.h"
#include "PlatformHelpers.h"
#include "BinaryReader.h"

#include "SDKMesh.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
    struct MaterialRecordSDKMESH
    {
        std::shared_ptr<IEffect> effect;
        bool alpha;
    };

    void LoadMaterial(_In_ const DXUT::SDKMESH_MATERIAL& mh,
        _In_ bool perVertexColor, _In_ bool enableSkinning, _In_ bool enableDualTexture, _In_ bool enableNormalMaps,
        _Inout_ IEffectFactory& fxFactory, _Inout_ MaterialRecordSDKMESH& m)
    {
        wchar_t matName[DXUT::MAX_MATERIAL_NAME];
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mh.Name, -1, matName, DXUT::MAX_MATERIAL_NAME);

        wchar_t diffuseName[DXUT::MAX_TEXTURE_NAME];
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mh.DiffuseTexture, -1, diffuseName, DXUT::MAX_TEXTURE_NAME);

        wchar_t specularName[DXUT::MAX_TEXTURE_NAME];
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mh.SpecularTexture, -1, specularName, DXUT::MAX_TEXTURE_NAME);

        wchar_t normalName[DXUT::MAX_TEXTURE_NAME];
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mh.NormalTexture, -1, normalName, DXUT::MAX_TEXTURE_NAME);

        if (enableDualTexture && !mh.SpecularTexture[0])
        {
            DebugTrace("WARNING: Material '%s' has multiple texture coords but not multiple textures\n", mh.Name);
            enableDualTexture = false;
        }

        if (enableNormalMaps)
        {
            if (!mh.NormalTexture[0])
            {
                enableNormalMaps = false;
                *normalName = 0;
            }
        }
        else if (mh.NormalTexture[0])
        {
            DebugTrace("WARNING: Material '%s' has a normal map, but vertex buffer is missing tangents\n", mh.Name);
            *normalName = 0;
        }

        EffectFactory::EffectInfo info;
        info.name = matName;
        info.perVertexColor = perVertexColor;
        info.enableSkinning = enableSkinning;
        info.enableDualTexture = enableDualTexture;
        info.enableNormalMaps = enableNormalMaps;
        info.ambientColor = XMFLOAT3(mh.Ambient.x, mh.Ambient.y, mh.Ambient.z);
        info.diffuseColor = XMFLOAT3(mh.Diffuse.x, mh.Diffuse.y, mh.Diffuse.z);
        info.emissiveColor = XMFLOAT3(mh.Emissive.x, mh.Emissive.y, mh.Emissive.z);

        if (mh.Diffuse.w != 1.f && mh.Diffuse.w != 0.f)
        {
            info.alpha = mh.Diffuse.w;
        }
        else
            info.alpha = 1.f;

        if (mh.Power)
        {
            info.specularPower = mh.Power;
            info.specularColor = XMFLOAT3(mh.Specular.x, mh.Specular.y, mh.Specular.z);
        }

        info.diffuseTexture = diffuseName;
        info.specularTexture = specularName;
        info.normalTexture = normalName;

        m.effect = fxFactory.CreateEffect(info, nullptr);
        m.alpha = (info.alpha < 1.f);
    }


    //--------------------------------------------------------------------------------------
    // Direct3D 9 Vertex Declaration to Direct3D 11 Input Layout mapping

    void GetInputLayoutDesc(_In_reads_(32) const DXUT::D3DVERTEXELEMENT9 decl[], std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDesc,
        bool &perVertexColor, bool& enableSkinning, bool& dualTexture, bool& normalMaps)
    {
        static const D3D11_INPUT_ELEMENT_DESC elements[] =
        {
            { "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",      0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",       0, DXGI_FORMAT_B8G8R8A8_UNORM,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BINORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BLENDINDICES",0, DXGI_FORMAT_R8G8B8A8_UINT,      0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "BLENDWEIGHT", 0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        using namespace DXUT;

        uint32_t offset = 0;
        uint32_t texcoords = 0;

        bool posfound = false;

        for (uint32_t index = 0; index < DXUT::MAX_VERTEX_ELEMENTS; ++index)
        {
            if (decl[index].Usage == 0xFF)
                break;

            if (decl[index].Type == D3DDECLTYPE_UNUSED)
                break;

            if (decl[index].Offset != offset)
                break;

            if (decl[index].Usage == D3DDECLUSAGE_POSITION && decl[index].Type == D3DDECLTYPE_FLOAT3)
            {
                inputDesc.push_back(elements[0]);
                offset += 12;
                posfound = true;
            }
            else if (decl[index].Usage == D3DDECLUSAGE_NORMAL)
            {
                if (decl[index].Type == D3DDECLTYPE_FLOAT3)
                {
                    inputDesc.push_back(elements[1]);
                    offset += 12;
                }
                else if (decl[index].Type == D3DDECLTYPE_FLOAT16_4)
                {
                    D3D11_INPUT_ELEMENT_DESC desc = elements[1];
                    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                    inputDesc.push_back(desc);
                    offset += 8;
                }
                else if (decl[index].Type == D3DDECLTYPE_SHORT4N)
                {
                    D3D11_INPUT_ELEMENT_DESC desc = elements[1];
                    desc.Format = DXGI_FORMAT_R16G16B16A16_SNORM;
                    inputDesc.push_back(desc);
                    offset += 8;
                }
                else if (decl[index].Type == D3DDECLTYPE_UBYTE4N)
                {
                    D3D11_INPUT_ELEMENT_DESC desc = elements[1];
                    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    inputDesc.push_back(desc);
                    offset += 4;
                }
                else
                    break;
            }
            else if (decl[index].Usage == D3DDECLUSAGE_COLOR && decl[index].Type == D3DDECLTYPE_D3DCOLOR)
            {
                inputDesc.push_back(elements[2]);
                offset += 4;
                perVertexColor = true;
            }
            else if (decl[index].Usage == D3DDECLUSAGE_TANGENT)
            {
                if (decl[index].Type == D3DDECLTYPE_FLOAT3)
                {
                    normalMaps = true;
                    inputDesc.push_back(elements[3]);
                    offset += 12;
                }
                else if (decl[index].Type == D3DDECLTYPE_FLOAT16_4)
                {
                    normalMaps = true;
                    D3D11_INPUT_ELEMENT_DESC desc = elements[3];
                    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                    inputDesc.push_back(desc);
                    offset += 8;
                }
                else if (decl[index].Type == D3DDECLTYPE_SHORT4N)
                {
                    normalMaps = true;
                    D3D11_INPUT_ELEMENT_DESC desc = elements[3];
                    desc.Format = DXGI_FORMAT_R16G16B16A16_SNORM;
                    inputDesc.push_back(desc);
                    offset += 8;
                }
                else if (decl[index].Type == D3DDECLTYPE_UBYTE4N)
                {
                    normalMaps = true;
                    D3D11_INPUT_ELEMENT_DESC desc = elements[3];
                    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    inputDesc.push_back(desc);
                    offset += 4;
                }
                else
                    break;
            }
            else if (decl[index].Usage == D3DDECLUSAGE_BINORMAL)
            {
                if (decl[index].Type == D3DDECLTYPE_FLOAT3)
                {
                    inputDesc.push_back(elements[4]);
                    offset += 12;
                }
                else if (decl[index].Type == D3DDECLTYPE_FLOAT16_4)
                {
                    D3D11_INPUT_ELEMENT_DESC desc = elements[4];
                    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                    inputDesc.push_back(desc);
                    offset += 8;
                }
                else if (decl[index].Type == D3DDECLTYPE_SHORT4N)
                {
                    D3D11_INPUT_ELEMENT_DESC desc = elements[4];
                    desc.Format = DXGI_FORMAT_R16G16B16A16_SNORM;
                    inputDesc.push_back(desc);
                    offset += 8;
                }
                else if (decl[index].Type == D3DDECLTYPE_UBYTE4N)
                {
                    D3D11_INPUT_ELEMENT_DESC desc = elements[4];
                    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    inputDesc.push_back(desc);
                    offset += 4;
                }
                else
                    break;
            }
            else if (decl[index].Usage == D3DDECLUSAGE_TEXCOORD)
            {
                D3D11_INPUT_ELEMENT_DESC desc = elements[5];
                desc.SemanticIndex = decl[index].UsageIndex;

                bool unk = false;
                switch (decl[index].Type)
                {
                case D3DDECLTYPE_FLOAT2:    offset += 8; break;
                case D3DDECLTYPE_FLOAT1:    desc.Format = DXGI_FORMAT_R32_FLOAT; offset += 4; break;
                case D3DDECLTYPE_FLOAT3:    desc.Format = DXGI_FORMAT_R32G32B32_FLOAT; offset += 12; break;
                case D3DDECLTYPE_FLOAT4:    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; offset += 16; break;
                case D3DDECLTYPE_FLOAT16_2: desc.Format = DXGI_FORMAT_R16G16_FLOAT; offset += 4; break;
                case D3DDECLTYPE_FLOAT16_4: desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; offset += 8; break;

                default:
                    unk = true;
                    break;
                }

                if (unk)
                    break;

                ++texcoords;

                inputDesc.push_back(desc);
            }
            else if (decl[index].Usage == D3DDECLUSAGE_BLENDINDICES && decl[index].Type == D3DDECLTYPE_UBYTE4)
            {
                enableSkinning = true;
                inputDesc.push_back(elements[6]);
                offset += 4;
            }
            else if (decl[index].Usage == D3DDECLUSAGE_BLENDWEIGHT && decl[index].Type == D3DDECLTYPE_UBYTE4N)
            {
                enableSkinning = true;
                inputDesc.push_back(elements[7]);
                offset += 4;
            }
            else
                break;
        }

        if (!posfound)
            throw std::exception("SV_Position is required");

        if (texcoords == 2)
        {
            dualTexture = true;
        }
    }

    // Helper for creating a D3D input layout.
    void CreateInputLayout(_In_ ID3D11Device* device, _In_ IEffect* effect, std::vector<D3D11_INPUT_ELEMENT_DESC>& inputDesc, _Out_ ID3D11InputLayout** pInputLayout)
    {
        void const* shaderByteCode;
        size_t byteCodeLength;

        effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        ThrowIfFailed(
            device->CreateInputLayout(inputDesc.data(),
                static_cast<UINT>(inputDesc.size()),
                shaderByteCode, byteCodeLength,
                pInputLayout)
        );

        _Analysis_assume_(*pInputLayout != 0);

        SetDebugObjectName(*pInputLayout, "ModelSDKMESH");
    }
}


//======================================================================================
// Model Loader
//======================================================================================

_Use_decl_annotations_
std::unique_ptr<Model> DirectX::Model::CreateFromSDKMESH( ID3D11Device* d3dDevice, const uint8_t* meshData, size_t dataSize, IEffectFactory& fxFactory, bool ccw, bool pmalpha )
{
    if ( !d3dDevice || !meshData )
        throw std::exception("Device and meshData cannot be null");

    // File Headers
    if ( dataSize < sizeof(DXUT::SDKMESH_HEADER) )
        throw std::exception("End of file");
    auto header = reinterpret_cast<const DXUT::SDKMESH_HEADER*>( meshData );

    size_t headerSize = sizeof( DXUT::SDKMESH_HEADER )
                        + header->NumVertexBuffers * sizeof(DXUT::SDKMESH_VERTEX_BUFFER_HEADER)
                        + header->NumIndexBuffers * sizeof(DXUT::SDKMESH_INDEX_BUFFER_HEADER);
    if ( header->HeaderSize != headerSize )
        throw std::exception("Not a valid SDKMESH file");

    if ( dataSize < header->HeaderSize )
        throw std::exception("End of file");

    if( header->Version != DXUT::SDKMESH_FILE_VERSION )
        throw std::exception("Not a supported SDKMESH version");
                          
    if ( header->IsBigEndian )
        throw std::exception("Loading BigEndian SDKMESH files not supported");

    if ( !header->NumMeshes )
        throw std::exception("No meshes found");

    if ( !header->NumVertexBuffers )
        throw std::exception("No vertex buffers found");

    if ( !header->NumIndexBuffers )
        throw std::exception("No index buffers found");

    if ( !header->NumTotalSubsets )
        throw std::exception("No subsets found");

    if ( !header->NumMaterials )
        throw std::exception("No materials found");

    // Sub-headers
    if ( dataSize < header->VertexStreamHeadersOffset
         || ( dataSize < (header->VertexStreamHeadersOffset + header->NumVertexBuffers * sizeof(DXUT::SDKMESH_VERTEX_BUFFER_HEADER) ) ) )
        throw std::exception("End of file");
    auto vbArray = reinterpret_cast<const DXUT::SDKMESH_VERTEX_BUFFER_HEADER*>( meshData + header->VertexStreamHeadersOffset );
    
    if ( dataSize < header->IndexStreamHeadersOffset
         || ( dataSize < (header->IndexStreamHeadersOffset + header->NumIndexBuffers * sizeof(DXUT::SDKMESH_INDEX_BUFFER_HEADER) ) ) )
        throw std::exception("End of file");
    auto ibArray = reinterpret_cast<const DXUT::SDKMESH_INDEX_BUFFER_HEADER*>( meshData + header->IndexStreamHeadersOffset );

    if ( dataSize < header->MeshDataOffset
         || ( dataSize < (header->MeshDataOffset + header->NumMeshes * sizeof(DXUT::SDKMESH_MESH) ) ) )
        throw std::exception("End of file");
    auto meshArray = reinterpret_cast<const DXUT::SDKMESH_MESH*>( meshData + header->MeshDataOffset );

    if ( dataSize < header->SubsetDataOffset
         || ( dataSize < (header->SubsetDataOffset + header->NumTotalSubsets * sizeof(DXUT::SDKMESH_SUBSET) ) ) )
        throw std::exception("End of file");
    auto subsetArray = reinterpret_cast<const DXUT::SDKMESH_SUBSET*>( meshData + header->SubsetDataOffset );

    if ( dataSize < header->FrameDataOffset
         || (dataSize < (header->FrameDataOffset + header->NumFrames * sizeof(DXUT::SDKMESH_FRAME) ) ) )
        throw std::exception("End of file");
    // TODO - auto frameArray = reinterpret_cast<const DXUT::SDKMESH_FRAME*>( meshData + header->FrameDataOffset );

    if ( dataSize < header->MaterialDataOffset
         || (dataSize < (header->MaterialDataOffset + header->NumMaterials * sizeof(DXUT::SDKMESH_MATERIAL) ) ) )
        throw std::exception("End of file");
    auto materialArray = reinterpret_cast<const DXUT::SDKMESH_MATERIAL*>( meshData + header->MaterialDataOffset );

    // Buffer data
    uint64_t bufferDataOffset = header->HeaderSize + header->NonBufferDataSize;
    if ( ( dataSize < bufferDataOffset )
         || ( dataSize < bufferDataOffset + header->BufferDataSize ) )
        throw std::exception("End of file");
    const uint8_t* bufferData = meshData + bufferDataOffset; 

    // Create vertex buffers
    std::vector<ComPtr<ID3D11Buffer>> vbs;
    vbs.resize( header->NumVertexBuffers );

    std::vector<std::shared_ptr<std::vector<D3D11_INPUT_ELEMENT_DESC>>> vbDecls;
    vbDecls.resize( header->NumVertexBuffers );

    std::vector<bool> perVertexColor;
    perVertexColor.resize( header->NumVertexBuffers );

    std::vector<bool> enableSkinning;
    enableSkinning.resize( header->NumVertexBuffers );

    std::vector<bool> enableDualTexture;
    enableDualTexture.resize( header->NumVertexBuffers );

    std::vector<bool> enableNormalMaps;
    enableNormalMaps.resize(header->NumVertexBuffers);

    for( UINT j=0; j < header->NumVertexBuffers; ++j )
    {
        auto& vh = vbArray[j];

        if ( dataSize < vh.DataOffset
             || ( dataSize < vh.DataOffset + vh.SizeBytes ) )
            throw std::exception("End of file");

        vbDecls[j] = std::make_shared<std::vector<D3D11_INPUT_ELEMENT_DESC>>();
        bool vertColor = false;
        bool skinning = false;
        bool dualTexture = false;
        bool normalMaps = false;
        GetInputLayoutDesc( vh.Decl, *vbDecls[j].get(), vertColor, skinning, dualTexture, normalMaps );
        perVertexColor[j] = vertColor;
        enableSkinning[j] = skinning;
        enableDualTexture[j] = !skinning && dualTexture;
        enableNormalMaps[j] = !skinning && !dualTexture && normalMaps;

        auto verts = reinterpret_cast<const uint8_t*>( bufferData + (vh.DataOffset - bufferDataOffset) );

        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ByteWidth = static_cast<UINT>( vh.SizeBytes );
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = verts;

        ThrowIfFailed(
            d3dDevice->CreateBuffer( &desc, &initData, &vbs[j] )
            );

        SetDebugObjectName( vbs[j].Get(), "ModelSDKMESH" ); 
    }

    // Create index buffers
    std::vector<ComPtr<ID3D11Buffer>> ibs;
    ibs.resize( header->NumIndexBuffers );
    
    for( UINT j=0; j < header->NumIndexBuffers; ++j )
    {
        auto& ih = ibArray[j];

        if ( dataSize < ih.DataOffset
             || ( dataSize < ih.DataOffset + ih.SizeBytes ) )
            throw std::exception("End of file");

        if ( ih.IndexType != DXUT::IT_16BIT && ih.IndexType != DXUT::IT_32BIT )
            throw std::exception("Invalid index buffer type found");

        auto indices = reinterpret_cast<const uint8_t*>( bufferData + (ih.DataOffset - bufferDataOffset) );

        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.ByteWidth = static_cast<UINT>( ih.SizeBytes );
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = indices;

        ThrowIfFailed(
            d3dDevice->CreateBuffer( &desc, &initData, &ibs[j] )
            );

        SetDebugObjectName( ibs[j].Get(), "ModelSDKMESH" ); 
    }

    // Create meshes
    std::vector<MaterialRecordSDKMESH> materials;
    materials.resize( header->NumMaterials );

    std::unique_ptr<Model> model(new Model());
    model->meshes.reserve( header->NumMeshes );

    for( UINT meshIndex = 0; meshIndex < header->NumMeshes; ++meshIndex )
    {
        auto& mh = meshArray[ meshIndex ];

        if ( !mh.NumSubsets
             || !mh.NumVertexBuffers
             || mh.IndexBuffer >= header->NumIndexBuffers
             || mh.VertexBuffers[0] >= header->NumVertexBuffers )
            throw std::exception("Invalid mesh found");

        // mh.NumVertexBuffers is sometimes not what you'd expect, so we skip validating it

        if ( dataSize < mh.SubsetOffset
             || (dataSize < mh.SubsetOffset + mh.NumSubsets*sizeof(UINT) ) )
            throw std::exception("End of file");

        auto subsets = reinterpret_cast<const UINT*>( meshData + mh.SubsetOffset );

        if ( mh.NumFrameInfluences > 0 )
        {
            if ( dataSize < mh.FrameInfluenceOffset
                 || (dataSize < mh.FrameInfluenceOffset + mh.NumFrameInfluences*sizeof(UINT) ) )
                throw std::exception("End of file");

            // TODO - auto influences = reinterpret_cast<const UINT*>( meshData + mh.FrameInfluenceOffset );
        }

        auto mesh = std::make_shared<ModelMesh>();
        wchar_t meshName[ DXUT::MAX_MESH_NAME ];
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, mh.Name, -1, meshName, DXUT::MAX_MESH_NAME );
        mesh->name = meshName;
        mesh->ccw = ccw;
        mesh->pmalpha = pmalpha;

        // Extents
        mesh->boundingBox.Center = mh.BoundingBoxCenter;
        mesh->boundingBox.Extents = mh.BoundingBoxExtents;
        BoundingSphere::CreateFromBoundingBox( mesh->boundingSphere, mesh->boundingBox );
       
        // Create subsets
        mesh->meshParts.reserve( mh.NumSubsets );
        for( UINT j = 0; j < mh.NumSubsets; ++j )
        {
            auto sIndex = subsets[ j ];
            if ( sIndex >= header->NumTotalSubsets )
                throw std::exception("Invalid mesh found");

            auto& subset = subsetArray[ sIndex ];

            D3D11_PRIMITIVE_TOPOLOGY primType;
            switch( subset.PrimitiveType )
            {
            case DXUT::PT_TRIANGLE_LIST:        primType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;       break;
            case DXUT::PT_TRIANGLE_STRIP:       primType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;      break;
            case DXUT::PT_LINE_LIST:            primType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;           break;
            case DXUT::PT_LINE_STRIP:           primType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;          break;
            case DXUT::PT_POINT_LIST:           primType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;          break;
            case DXUT::PT_TRIANGLE_LIST_ADJ:    primType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;   break;
            case DXUT::PT_TRIANGLE_STRIP_ADJ:   primType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;  break;
            case DXUT::PT_LINE_LIST_ADJ:        primType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;       break;
            case DXUT::PT_LINE_STRIP_ADJ:       primType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;      break;

            case DXUT::PT_QUAD_PATCH_LIST:
            case DXUT::PT_TRIANGLE_PATCH_LIST:
                throw std::exception("Direct3D9 era tessellation not supported");

            default:
                throw std::exception("Unknown primitive type");
            }

            if ( subset.MaterialID >= header->NumMaterials )
                throw std::exception("Invalid mesh found");

            auto& mat = materials[ subset.MaterialID ];

            if ( !mat.effect )
            {
                size_t vi = mh.VertexBuffers[0];
                LoadMaterial( materialArray[ subset.MaterialID ],
                              perVertexColor[vi], enableSkinning[vi], enableDualTexture[vi], enableNormalMaps[vi],
                              fxFactory, mat );
            }

            ComPtr<ID3D11InputLayout> il;
            CreateInputLayout( d3dDevice, mat.effect.get(), *vbDecls[ mh.VertexBuffers[0] ].get(), &il );

            auto part = new ModelMeshPart();
            part->isAlpha = mat.alpha;

            part->indexCount = static_cast<uint32_t>( subset.IndexCount );
            part->startIndex = static_cast<uint32_t>( subset.IndexStart );
            part->vertexOffset = static_cast<uint32_t>( subset.VertexStart );
            part->vertexStride = static_cast<uint32_t>( vbArray[ mh.VertexBuffers[0] ].StrideBytes );
            part->indexFormat = ( ibArray[ mh.IndexBuffer ].IndexType == DXUT::IT_32BIT ) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
            part->primitiveType = primType; 
            part->inputLayout = il;
            part->indexBuffer = ibs[ mh.IndexBuffer ];
            part->vertexBuffer = vbs[ mh.VertexBuffers[0] ];
            part->effect = mat.effect;
            part->vbDecl = vbDecls[ mh.VertexBuffers[0] ];

            mesh->meshParts.emplace_back( part );
        }

        model->meshes.emplace_back( mesh );
    }

    return model;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
std::unique_ptr<Model> DirectX::Model::CreateFromSDKMESH( ID3D11Device* d3dDevice, const wchar_t* szFileName, IEffectFactory& fxFactory, bool ccw, bool pmalpha )
{
    size_t dataSize = 0;
    std::unique_ptr<uint8_t[]> data;
    HRESULT hr = BinaryReader::ReadEntireFile( szFileName, data, &dataSize );
    if ( FAILED(hr) )
    {
        DebugTrace( "CreateFromSDKMESH failed (%08X) loading '%ls'\n", hr, szFileName );
        throw std::exception( "CreateFromSDKMESH" );
    }

    auto model = CreateFromSDKMESH( d3dDevice, data.get(), dataSize, fxFactory, ccw, pmalpha );

    model->name = szFileName;

    return model;
}
