#pragma once

#include "RenderElement.h"
#include "Material.h"
#include "common/AsyncExecutor/AsyncAgent.h"
#include "Model/ModelImage.h"
#include "Model/ModelMesh.h"

namespace rayr
{

class alignas(16) Model : public Drawable
{
protected:
    void InitMaterial();
public:
    static constexpr auto TYPENAME = u"Model";
    ModelMesh Mesh;
    Model(const u16string& fname, bool asyncload = false);
    ~Model() override;
    virtual void PrepareGL(const oglu::oglProgram& prog, const map<string, string>& translator = map<string, string>()) override;
    virtual void Draw(Drawcall& drawcall) const override;
};

}