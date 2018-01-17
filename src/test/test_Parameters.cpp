#include "testing/OriTestBase.h"
#include "../core/Parameters.h"
#include "TestUtils.h"

namespace Z {
namespace Test {
namespace ParametersTests {

//------------------------------------------------------------------------------

TEST_METHOD(Parameter_ctor_default)
{
    Z::Parameter p;
    ASSERT_EQ_PTR(p.dim(), Z::Dims::none())
    ASSERT_IS_FALSE(p.visible())
}

TEST_METHOD(Parameter_ctor_params)
{
    Z::Parameter p(Z::Dims::linear(), "alias", "label", "name", "description", "category", true);
    ASSERT_EQ_PTR(p.dim(), Z::Dims::linear())
    ASSERT_EQ_STR(p.alias(), "alias")
    ASSERT_EQ_STR(p.label(), "label")
    ASSERT_EQ_STR(p.name(), "name")
    ASSERT_EQ_STR(p.description(), "description")
    ASSERT_EQ_STR(p.category(), "category")
    ASSERT_IS_TRUE(p.visible())
}

TEST_METHOD(Parameter_setValue_getValue)
{
    Z::Parameter p(Z::Dims::linear(), "", "", "");

    p.setValue(Z::Value(100, Z::Units::m()));
    ASSERT_Z_VALUE_AND_UNIT(p.value(), 100, Z::Units::m())

    p.setValue(Z::Value(3.14, Z::Units::mm()));
    ASSERT_Z_VALUE_AND_UNIT(p.value(), 3.14, Z::Units::mm())
}

//------------------------------------------------------------------------------

class TestParamListener : public Z::ParameterListener
{
public:
    QString changedParam;

    void parameterChanged(Z::ParameterBase *p) override
    {
        changedParam = p->name();
    }
};

TEST_METHOD(ParameterListener_parameterChanged)
{
    TestParamListener listener;
    Z::Parameter p(Z::Dims::linear(), "", "param1", "");
    p.addListener(&listener);

    listener.changedParam.clear();
    p.setValue(Z::Value(100, Z::Units::mm()));
    ASSERT_EQ_STR(listener.changedParam, p.name());
}

//------------------------------------------------------------------------------

TEST_METHOD(Parameters_byAlias)
{
    Z::Parameter p1(Z::Dims::none(), "p1", "", "");
    Z::Parameter p2(Z::Dims::none(), "p2", "", "");
    Z::Parameters params { &p1, &p2 };
    ASSERT_EQ_PTR(params.byAlias("p1"), &p1);
    ASSERT_EQ_PTR(params.byAlias("p2"), &p2);
    ASSERT_IS_NULL(params.byAlias("p3"));
}

TEST_METHOD(Parameters_byIndex)
{
    Z::Parameter p1(Z::Dims::none(), "p1", "", "");
    Z::Parameter p2(Z::Dims::none(), "p2", "", "");
    Z::Parameters params { &p1, &p2 };
    ASSERT_EQ_PTR(params.byIndex(0), &p1);
    ASSERT_EQ_PTR(params.byIndex(1), &p2);
    ASSERT_IS_NULL(params.byIndex(2));
}

//------------------------------------------------------------------------------

namespace ParameterFilterTests {

TEST_METHOD(ParameterFilterVisible_check)
{
    Z::ParameterFilterVisible filter;

    Z::Parameter p;
    ASSERT_IS_FALSE(filter.check(&p));

    p.setVisible(true);
    ASSERT_IS_TRUE(filter.check(&p));
}

TEST_GROUP("ParameterFilter",
    ADD_TEST(ParameterFilterVisible_check)
)

} // namespace ParameterFilterTests

//------------------------------------------------------------------------------

TEST_GROUP("Parameters",
    ADD_TEST(Parameter_ctor_default),
    ADD_TEST(Parameter_ctor_params),
    ADD_TEST(Parameter_setValue_getValue),
    ADD_TEST(ParameterListener_parameterChanged),
    ADD_TEST(Parameters_byAlias),
    ADD_TEST(Parameters_byIndex),
    ADD_GROUP(ParameterFilterTests),
)

} // namespace ParametersTests
} // namespace Test
} // namespace Z
