#include <Gfx/Graph/Mesh.hpp>
#include <Gfx/Graph/NodeRenderer.hpp>
#include <Gfx/Graph/RenderList.hpp>
#include <Gfx/Graph/OutputNode.hpp>

namespace score::gfx
{
#include <Gfx/Qt5CompatPush> // clang-format: keep
MeshBuffers RenderList::initMeshBuffer(const Mesh& mesh)
{
  if (auto it = m_vertexBuffers.find(&mesh); it != m_vertexBuffers.end())
    return it->second;

  auto& rhi = *state.rhi;
  auto mesh_buf = rhi.newBuffer(
      QRhiBuffer::Immutable,
      QRhiBuffer::VertexBuffer,
      mesh.vertexArray.size() * sizeof(float));
  mesh_buf->setName("RenderList::mesh_buf");
  mesh_buf->create();


  QRhiBuffer* idx_buf{};
  if (!mesh.indexArray.empty())
  {
    idx_buf = rhi.newBuffer(
        QRhiBuffer::Immutable,
        QRhiBuffer::IndexBuffer,
        mesh.indexArray.size() * sizeof(unsigned int));
    idx_buf->setName("RenderList::idx_buf");
    idx_buf->create();
  }

  MeshBuffers ret{mesh_buf, idx_buf};
  m_vertexBuffers.insert({&mesh, ret});
  buffersToUpload.push_back({&mesh, ret});
  return ret;
}

RenderList::RenderList(OutputNode& output, const RenderState& state)
    : output{output}
    , state{state}
{
  output.setRenderer(this);
}

void RenderList::init()
{
  m_ready = false;
  if (!state.rhi)
    return;
  auto& rhi = *state.rhi;

  m_outputUBO = rhi.newBuffer(
      QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(OutputUBO));
  m_outputUBO->setName("RenderList::m_outputUBO");
  m_outputUBO->create();

  m_emptyTexture = rhi.newTexture(
      QRhiTexture::RGBA8, QSize{1, 1}, 1, QRhiTexture::Flag{});
  m_emptyTexture->setName("RenderList::m_emptyTexture");
  m_emptyTexture->create();
}

void RenderList::release()
{
  for (auto node : renderers)
    node->release(*this);

  for (auto bufs : m_vertexBuffers)
  {
    delete bufs.second.mesh;
    delete bufs.second.index;
  }

  m_vertexBuffers.clear();
  buffersToUpload.clear();

  delete m_outputUBO;
  m_outputUBO = nullptr;

  delete m_emptyTexture;
  m_emptyTexture = nullptr;

  m_ready = false;
}

void RenderList::maybeRebuild()
{
  const QSize outputSize = state.size;
  if (outputSize != m_lastSize)
  {
    release();

    // Now we have the nodes in the order in which they are going to
    // be processed

    init();

    // We init the nodes in reverse orders as
    // the render targets of subsequent nodes must be initialized
    for (auto it = renderers.rbegin(); it != renderers.rend(); ++it)
    {
      (*it)->init(*this);
    }

    m_lastSize = outputSize;
  }
}

TextureRenderTarget RenderList::renderTargetForOutput(Port& port) noexcept
{
  auto missing = [this] {
    SCORE_ASSERT(!this->renderers.empty());
    auto output_renderer = this->renderers.back();
    SCORE_ASSERT(this->output.input.size() > 0);
    return output_renderer->renderTargetForInput(*this->output.input[0]);
  };
  if (port.edges.empty())
    return missing();

  SCORE_TODO_("what if there are multiple outputs for a node");
  SCORE_TODO_("also we must check only the edges that are part of this output");
  auto edge = port.edges[0];
  auto sink_node = edge->sink->node;
  if (!sink_node)
    return missing();

  auto renderer = sink_node->renderedNodes[this];
  if (!renderer)
    return missing();

  if (auto tex = renderer->renderTargetForInput(*edge->sink);
      tex.renderTarget && tex.renderPass)
    return tex;
  else
    return missing();
}

/*
QRhiTexture* RenderList::textureTargetForInputPort(Port& port)
{
  QRhiTexture* texture = m_emptyTexture;
  if (port.edges.empty())
    return texture;

  auto source_node = port.edges[0]->source->node;
  if (!source_node)
    return texture;

  auto renderer = source_node->renderedNodes[this];
  if (!renderer)
    return texture;

  if (auto tex = renderer->renderTarget().texture)
    return tex;
  else
    return texture;
}
*/

void RenderList::render(QRhiCommandBuffer& commands)
{
  if (renderers.size() <= 1)
  {
    return;
  }

  // Check if the viewport has changed
  maybeRebuild();

  auto updateBatch = state.rhi->nextResourceUpdateBatch();
  update(*updateBatch);

  // For each texture input port
  //  For all previous node
  //   Update
  //  Begin pass
  //   For all previous node
  //    Render
  //  End pass

  qDebug() << "Begin render";

  for(auto node : this->nodes)
  {
    for(auto input : node->input)
    {
      if(input->type == Types::Image)
      {
        for(auto edge : input->edges)
        {
          auto src = edge->source;
          SCORE_ASSERT(src);

          SCORE_ASSERT(src->node->renderedNodes.find(this) != src->node->renderedNodes.end());
          auto renderer = src->node->renderedNodes[this];


          qDebug() << "update: " << typeid(*src->node).name();
          renderer->update(*this, *updateBatch);
        }

        SCORE_ASSERT(node->renderedNodes.find(this) != node->renderedNodes.end());
        auto m_rt = node->renderedNodes[this]->renderTargetForInput(*input);
        SCORE_ASSERT(m_rt.renderTarget);
        commands.beginPass(m_rt.renderTarget, Qt::black, {1.0f, 0}, updateBatch);

        for(auto edge : input->edges)
        {
          auto src = edge->source;
          SCORE_ASSERT(src);

          SCORE_ASSERT(src->node->renderedNodes.find(this) != src->node->renderedNodes.end());
          auto renderer = src->node->renderedNodes[this];

          qDebug() << "runPass: " << typeid(*src->node).name();
          renderer->runPass(*this, commands, *updateBatch);
        }

        commands.endPass();

        if(node != &this->output)
        {
          qDebug() << "Update updatebatch";
          updateBatch = state.rhi->nextResourceUpdateBatch();
        }
      }
    }
  }

  qDebug() << "End render";
/*
  for (std::size_t i = 0; i < renderers.size(); i++)
  {
    renderers[i]->runPass(*this, commands, *updateBatch);

    if (i < renderers.size() - 1)
      updateBatch = state.rhi->nextResourceUpdateBatch();
  }*/
}

void RenderList::update(QRhiResourceUpdateBatch& res)
{
  if (!m_ready)
  {
    m_ready = true;

    const auto proj = state.rhi->clipSpaceCorrMatrix();

    if (!state.rhi->isYUpInFramebuffer())
    {
      // Vulkan, D3D, Metal
      m_outputUBOData.texcoordAdjust[0] = 1.f;
      m_outputUBOData.texcoordAdjust[1] = 0.f;
    }
    else
    {
      // GL
      m_outputUBOData.texcoordAdjust[0] = -1.f;
      m_outputUBOData.texcoordAdjust[1] = 1.f;
    }

    memcpy(&m_outputUBOData.clipSpaceCorrMatrix[0], proj.data(), sizeof(float) * 16);

    m_outputUBOData.renderSize[0] = this->m_lastSize.width();
    m_outputUBOData.renderSize[1] = this->m_lastSize.height();

    res.updateDynamicBuffer(m_outputUBO, 0, sizeof(OutputUBO), &m_outputUBOData);
  }

  if (Q_UNLIKELY(!buffersToUpload.empty()))
  {
    for (auto [mesh, buf] : buffersToUpload)
    {
      res.uploadStaticBuffer(
          buf.mesh, 0, buf.mesh->size(), mesh->vertexArray.data());
      if (buf.index)
        res.uploadStaticBuffer(
            buf.index, 0, buf.index->size(), mesh->indexArray.data());
    }

    buffersToUpload.clear();
  }
}

#include <Gfx/Qt5CompatPop> // clang-format: keep
}
