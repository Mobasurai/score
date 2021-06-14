#include <Gfx/Graph/VideoNode.hpp>
#include <Gfx/Graph/VideoNodeRenderer.hpp>

namespace score::gfx
{
VideoNode::VideoNode(
    std::shared_ptr<Video::VideoInterface> dec,
    std::optional<double> nativeTempo,
    QString f)
    : m_decoder{std::move(dec)}
    , m_nativeTempo{nativeTempo}
    , m_filter{f}
{
  output.push_back(new Port{this, {}, Types::Image, {}});
}

const Mesh& VideoNode::mesh() const noexcept
{
  return this->m_mesh;
}

VideoNode::~VideoNode() { }

score::gfx::NodeRenderer*
VideoNode::createRenderer(RenderList& r) const noexcept
{
  return nullptr;//new VideoNodeRenderer{*this};
}

void VideoNode::seeked()
{
  SCORE_TODO;
}
}
#include <hap/source/hap.c>
