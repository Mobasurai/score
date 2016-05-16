#pragma once
#include <iscore/document/DocumentContext.hpp>

class FocusDispatcher;
namespace Process
{
class LayerPresenter;

struct ProcessPresenterContext : public iscore::DocumentContext
{
        ProcessPresenterContext(
                const iscore::DocumentContext& doc,
                FocusDispatcher& d):
            iscore::DocumentContext{doc},
            focusDispatcher{d}
        {

        }

        FocusDispatcher& focusDispatcher;
};

struct LayerContext
{/*
        LayerContext(
                iscore::Document& doc,
                Process::LayerPresenter& pres,
                ProcessPresenterContext& d):
            iscore::DocumentContext{doc},
            layerPresenter{pres},
            focusDispatcher{d}
        {

        }
*/
        const ProcessPresenterContext& context;
        Process::LayerPresenter& layerPresenter;
};
}
