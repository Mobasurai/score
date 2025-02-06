#pragma once

#include <Gfx/Graph/NodeRenderer.hpp>
#include <Gfx/Graph/OutputNode.hpp>
namespace Gfx
{

class SCORE_PLUGIN_GFX_EXPORT InvertYRenderer final
    : public score::gfx::OutputNodeRenderer
{
public:
  explicit InvertYRenderer(
      const score::gfx::Node& n, score::gfx::TextureRenderTarget rt,
      QRhiReadbackResult& readback);

  score::gfx::TextureRenderTarget m_inputTarget;
  score::gfx::TextureRenderTarget m_renderTarget;

  QShader m_vertexS, m_fragmentS;

  std::vector<score::gfx::Sampler> m_samplers;

  score::gfx::Pipeline m_p;

  score::gfx::MeshBuffers m_mesh{};

  score::gfx::TextureRenderTarget
  renderTargetForInput(const score::gfx::Port& p) override
  {
    return m_inputTarget;
  }

  void finishFrame(
      score::gfx::RenderList& renderer, QRhiCommandBuffer& cb,
      QRhiResourceUpdateBatch*& res) override;

  void init(score::gfx::RenderList& renderer, QRhiResourceUpdateBatch& res) override;
  void update(
      score::gfx::RenderList& renderer, QRhiResourceUpdateBatch& res,
      score::gfx::Edge* edge) override;
  void release(score::gfx::RenderList&) override;

  void updateReadback(QRhiReadbackResult& rb) { m_readback = &rb; }

private:
  QRhiReadbackResult* m_readback{};
};

}
