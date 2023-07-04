#pragma once

#if SCORE_PLUGIN_GFX
#include <Crousti/GfxNode.hpp>

namespace oscr
{

template <typename Node_T>
  requires(
      avnd::texture_input_introspection<Node_T>::size > 0
      && avnd::texture_output_introspection<Node_T>::size > 0)
struct GfxRenderer<Node_T> final : score::gfx::GenericNodeRenderer
{
  using texture_inputs = avnd::texture_input_introspection<Node_T>;
  using texture_outputs = avnd::texture_output_introspection<Node_T>;
  const GfxNode<Node_T>& parent;
  Node_T state;
  ossia::small_flat_map<const score::gfx::Port*, score::gfx::TextureRenderTarget, 2>
      m_rts;

  std::vector<QRhiReadbackResult> m_readbacks;
  ossia::time_value m_last_time{-1};

  GfxRenderer(const GfxNode<Node_T>& p)
      : score::gfx::GenericNodeRenderer{p}
      , parent{p}
      , m_readbacks(texture_inputs::size)
  {
    prepareNewState(state, parent);
  }

  score::gfx::TextureRenderTarget
  renderTargetForInput(const score::gfx::Port& p) override
  {
    auto it = m_rts.find(&p);
    SCORE_ASSERT(it != m_rts.end());
    return it->second;
  }

  template <typename Tex>
  void createInput(
      score::gfx::RenderList& renderer, int k, const Tex& texture_spec, QSize size)
  {
    auto port = parent.input[k];
    constexpr auto flags = QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource;
    auto texture = renderer.state.rhi->newTexture(
        gpp::qrhi::textureFormat<Tex>(), size, 1, flags);
    SCORE_ASSERT(texture->create());
    m_rts[port]
        = score::gfx::createRenderTarget(renderer.state, texture, renderer.samples());
  }

  template <typename Tex>
  void
  createOutput(score::gfx::RenderList& renderer, const Tex& texture_spec, QSize size)
  {
    auto& rhi = *renderer.state.rhi;
    QRhiTexture* texture = &renderer.emptyTexture();
    if(size.width() > 0 && size.height() > 0)
    {
      texture = rhi.newTexture(
          gpp::qrhi::textureFormat<Tex>(), size, 1, QRhiTexture::Flag{});

      texture->create();
    }

    auto sampler = rhi.newSampler(
        QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);

    sampler->create();
    this->m_samplers.push_back({sampler, texture});
  }

  template <avnd::cpu_texture Tex>
  QRhiTexture* updateTexture(score::gfx::RenderList& renderer, int k, const Tex& cpu_tex)
  {
    auto& [sampler, texture] = this->m_samplers[k];
    if(texture)
    {
      auto sz = texture->pixelSize();
      if(cpu_tex.width == sz.width() && cpu_tex.height == sz.height())
        return texture;
    }

    // Check the texture size
    if(cpu_tex.width > 0 && cpu_tex.height > 0)
    {
      QRhiTexture* oldtex = texture;
      QRhiTexture* newtex = renderer.state.rhi->newTexture(
          gpp::qrhi::textureFormat<Tex>(), QSize{cpu_tex.width, cpu_tex.height}, 1,
          QRhiTexture::Flag{});
      newtex->create();
      for(auto& [edge, pass] : this->m_p)
        if(pass.srb)
          score::gfx::replaceTexture(*pass.srb, sampler, newtex);
      texture = newtex;

      if(oldtex && oldtex != &renderer.emptyTexture())
      {
        oldtex->deleteLater();
      }

      return newtex;
    }
    else
    {
      for(auto& [edge, pass] : this->m_p)
        if(pass.srb)
          score::gfx::replaceTexture(*pass.srb, sampler, &renderer.emptyTexture());

      return &renderer.emptyTexture();
    }
  }

  void uploadOutputTexture(
      score::gfx::RenderList& renderer, int k, avnd::cpu_texture auto& cpu_tex,
      QRhiResourceUpdateBatch* res)
  {
    if(cpu_tex.changed)
    {
      if(auto texture = updateTexture(renderer, k, cpu_tex))
      {
        // Upload it
        {
          QRhiTextureSubresourceUploadDescription sd(
              cpu_tex.bytes, cpu_tex.width * cpu_tex.height * 4);
          QRhiTextureUploadDescription desc{QRhiTextureUploadEntry{0, 0, sd}};

          res->uploadTexture(texture, desc);
        }

        cpu_tex.changed = false;
        k++;
      }
    }
  }

  void loadInputTexture(avnd::cpu_texture auto& cpu_tex, int k)
  {
    auto& buf = m_readbacks[k].data;
    if(buf.size() != 4 * cpu_tex.width * cpu_tex.height)
    {
      cpu_tex.bytes = nullptr;
    }
    else
    {
      cpu_tex.bytes = reinterpret_cast<unsigned char*>(buf.data());
      cpu_tex.changed = true;
    }
  }

  void init(score::gfx::RenderList& renderer, QRhiResourceUpdateBatch& res) override
  {
    const auto& mesh = renderer.defaultTriangle();
    this->defaultMeshInit(renderer, mesh, res);
    this->processUBOInit(renderer);
    this->m_material.init(renderer, this->node.input, this->m_samplers);
    std::tie(this->m_vertexS, this->m_fragmentS)
        = score::gfx::makeShaders(renderer.state, generic_texgen_vs, generic_texgen_fs);

    {
      // Init input render targets
      int k = 0;
      avnd::cpu_texture_input_introspection<Node_T>::for_all(
          avnd::get_inputs<Node_T>(state), [&]<typename F>(F& t) {
            auto sz = renderer.state.renderSize;
            createInput(renderer, k, t.texture, sz);
            t.texture.width = sz.width();
            t.texture.height = sz.height();
            k++;
          });
    }

    {
      // Init textures for the outputs
      avnd::cpu_texture_output_introspection<Node_T>::for_all(
          avnd::get_outputs<Node_T>(state), [&](auto& t) {
            createOutput(renderer, t.texture, QSize{t.texture.width, t.texture.height});
          });
    }

    this->defaultPassesInit(renderer, mesh);
  }

  void update(score::gfx::RenderList& renderer, QRhiResourceUpdateBatch& res) override
  {
    this->defaultUBOUpdate(renderer, res);
  }

  void release(score::gfx::RenderList& r) override
  {
    // Free outputs
    for(auto& [sampl, texture] : this->m_samplers)
    {
      if(texture != &r.emptyTexture())
        texture->deleteLater();
      texture = nullptr;
    }

    // Free inputs
    // TODO investigate why reference does not work here:
    for(auto [port, rt] : m_rts)
      rt.release();
    m_rts.clear();

    this->defaultRelease(r);
  }

  void inputAboutToFinish(
      score::gfx::RenderList& renderer, const score::gfx::Port& p,
      QRhiResourceUpdateBatch*& res) override
  {
    res = renderer.state.rhi->nextResourceUpdateBatch();
    const auto& inputs = this->node.input;
    auto index_of_port = ossia::find(inputs, &p) - inputs.begin();
    SCORE_ASSERT(index_of_port == 0);
    {
      auto tex = m_rts[&p].texture;
      auto& readback = m_readbacks[index_of_port];
      readback = {};
      res->readBackTexture(QRhiReadbackDescription{tex}, &readback);
    }
  }

  void runInitialPasses(
      score::gfx::RenderList& renderer, QRhiCommandBuffer& commands,
      QRhiResourceUpdateBatch*& res, score::gfx::Edge& edge) override
  {
    auto& rhi = *renderer.state.rhi;

    // If we are paused, we don't run the processor implementation.
    if(parent.last_message.token.date == m_last_time)
    {
      return;
    }
    m_last_time = parent.last_message.token.date;

    // Fetch input textures (if any)
    {
      // Insert a synchronisation point to allow readbacks to complete
      rhi.finish();

      // Copy the readback output inside the structure
      // TODO it would be much better to do this inside the readback's
      // "completed" callback.
      int k = 0;
      avnd::cpu_texture_input_introspection<Node_T>::for_all(
          avnd::get_inputs<Node_T>(state), [&](auto& t) {
            loadInputTexture(t.texture, k);
            k++;
          });
    }

    // Apply the controls
    avnd::parameter_input_introspection<Node_T>::for_all_n2(
        avnd::get_inputs<Node_T>(state),
        [&](avnd::parameter auto& t, auto pred_index, auto field_index) {
      auto& mess = this->parent.last_message;

      if(mess.input.size() > field_index)
      {
        if(auto val = ossia::get_if<ossia::value>(&mess.input[field_index]))
        {
          oscr::from_ossia_value(t, *val, t.value);
        }
      }
        });

    // Run the processor
    state();

    // Upload output textures
    {
      int k = 0;
      avnd::cpu_texture_output_introspection<Node_T>::for_all(
          avnd::get_outputs<Node_T>(state), [&](auto& t) {
            uploadOutputTexture(renderer, k, t.texture, res);
            k++;
          });

      commands.resourceUpdate(res);
      res = renderer.state.rhi->nextResourceUpdateBatch();
    }

    // Copy the data to the model node
    parent.processControlOut(this->state);
  }
};


template <typename Node_T>
  requires(avnd::texture_input_introspection<Node_T>::size > 0
           && avnd::texture_output_introspection<Node_T>::size > 0)
struct GfxNode<Node_T> final
    : CustomGfxNodeBase
    , GpuControlOuts
{
  GfxNode(
      std::weak_ptr<Execution::ExecutionCommandQueue> q, Gfx::exec_controls ctls, int id)
      : GpuControlOuts{std::move(q), std::move(ctls)}
  {
    this->instance = id;

    using texture_inputs = avnd::texture_input_introspection<Node_T>;
    using texture_outputs = avnd::texture_output_introspection<Node_T>;

    // FIXME incorrect if we have other ports before, e.g. a float part followed by an image port
    for(std::size_t i = 0; i < texture_inputs::size; i++)
    {
      this->input.push_back(
          new score::gfx::Port{this, {}, score::gfx::Types::Image, {}});
    }
    for(std::size_t i = 0; i < texture_outputs::size; i++)
    {
      this->output.push_back(
          new score::gfx::Port{this, {}, score::gfx::Types::Image, {}});
    }
  }

  score::gfx::NodeRenderer*
  createRenderer(score::gfx::RenderList& r) const noexcept override
  {
    return new GfxRenderer<Node_T>{*this};
  }
};

}
#endif
