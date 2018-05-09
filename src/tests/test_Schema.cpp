#include "testing/OriTestBase.h"
#include "../core/Schema.h"
#include "TestUtils.h"

#include <memory>

namespace Z {
namespace Tests {
namespace SchemaTests {

class TestSchemaListener : public SchemaListener
{
public:
    void schemaCreated(Schema*s) { store(SchemaEvents::Created, s); }
    void schemaDeleted(Schema*s) { store(SchemaEvents::Deleted, s); }
    void schemaChanged(Schema*s) { store(SchemaEvents::Changed, s); }
    void schemaSaved(Schema*s) { store(SchemaEvents::Saved, s); }
    void schemaLoading(Schema*s) { store(SchemaEvents::Loading, s); }
    void schemaLoaded(Schema*s) { store(SchemaEvents::Loaded, s); }
    void elementCreated(Schema*s, Element*e) { store(SchemaEvents::ElemCreated, s, e); }
    void elementDeleting(Schema*s, Element*e) { store(SchemaEvents::ElemDeleting, s, e); }
    void elementDeleted(Schema*s, Element*e) { store(SchemaEvents::ElemDeleted, s, e); }
    void elementChanged(Schema*s, Element*e) { store(SchemaEvents::ElemChanged, s, e); }
    void schemaParamsChanged(Schema*s) { store(SchemaEvents::ParamsChanged, s); }
    void schemaLambdaChanged(Schema*s) { store(SchemaEvents::LambdaChanged, s); }

public:
    Schema *schema = nullptr;
    Element *element = nullptr;
    QVector<SchemaEvents::Event> events;

    void reset()
    {
        schema = nullptr;
        element = nullptr;
        events.clear();
    }

    bool checkEvents(QVector<SchemaEvents::Event> expected)
    {
        if (events.size() != expected.size()) return false;
        for (int i = 0; i < events.size(); i++)
            if (events.at(i) != expected.at(i)) return false;
        return true;
    }

    QString eventsStr() const
    {
        QStringList s;
        for (auto event : events)
            s << "    " % SchemaEvents::str(event);
        return "Captured events:\n" % (s.isEmpty()? "    (none)": s.join("\n")) % "\n";
    }

private:
    void store(SchemaEvents::Event event, Schema *s, Element *e = nullptr)
    {
        schema = s;
        if (e)
            element = e;
        events << event;
    }
};

#define ASSERT_ELEM_COUNT(expected_count)\
    ASSERT_EQ_INT(schema.count(), expected_count)

#define TAKE_ELEM_PTR(elem_var, index)\
    std::unique_ptr<Element> auto_##elem_var(schema.element(index));\
    auto elem_var = auto_##elem_var.get();\
    Q_UNUSED(elem_var)

#define ASSERT_SCHEMA_STATE(expected_state)\
    ASSERT_EQ_INT(int(schema.state().current()), int(expected_state))

// Listener should be created before schema, to be destroyed in last turn.
// ~Schema() accesses listener, so schema must be destroyed first.
#define SCHEMA_AND_LISTENER \
    TestSchemaListener listener; \
    Schema schema; \
    schema.registerListener(&listener);

#define ASSERT_LISTENER(expected_elem, ...) { \
    ASSERT_IS_TRUE(listener.schema == &schema) \
    /*TEST_LOG_PTR(listener.element)*/\
    /*TEST_LOG_PTR(expected_elem)*/\
    ASSERT_IS_TRUE(listener.element == expected_elem) \
    TEST_LOG(listener.eventsStr())\
    ASSERT_IS_TRUE(listener.checkEvents({__VA_ARGS__})) \
}

#define EVENT(event) SchemaEvents::event
#define STATE(state) SchemaState::state

#define SCHEMA_RESET_STATE\
    schema.events().raise(SchemaEvents::Saved);

#define ASSERT_LISTENER_NO_EVENTS \
    TEST_LOG(listener.eventsStr())\
    ASSERT_IS_TRUE(listener.events.isEmpty())

//------------------------------------------------------------------------------

DECLARE_ELEMENT(TestElement, Element)
    Ori::Testing::TestBase *test = nullptr;
    ~TestElement()
    {
        if (!test) return;
        TEST_LOG("~TestElement() " + label())
        test->data().insert(label(), true);
    }
DECLARE_ELEMENT_END

#define PREPARE_SCHEMA_ELEMS(elem_count)\
    SCHEMA_AND_LISTENER \
    for (int i = 0; i < elem_count; i++ )\
        schema.insertElement(new TestElement, -1, false);

//------------------------------------------------------------------------------

TEST_METHOD(constructor)
{
    Schema schema;
    ASSERT_SCHEMA_STATE(STATE(New))
    ASSERT_IS_FALSE(schema.modified())
}

TEST_METHOD(destructor_must_delete_elements)
{
    auto schema = new Schema;
    for (int i = 0; i < 10; i++)
    {
        auto elem = new TestElement;
        elem->setLabel(QString::number(i));
        elem->test = test;
        schema->insertElement(elem);
    }
    delete schema;
    for (int i = 0; i < 10; i++)
        ASSERT_IS_TRUE(test->data()[QString::number(i)].toBool()) // element was deleted
}

TEST_METHOD(destructor_must_raise_event)
{
    TestSchemaListener listener;
    auto schema = new Schema;
    schema->registerListener(&listener);
    delete schema;
    ASSERT_IS_TRUE(listener.schema == schema)
    ASSERT_IS_TRUE(listener.checkEvents({EVENT(Deleted)}))
}

TEST_METHOD(destructor_with_no_events)
{
    TestSchemaListener listener;
    auto schema = new Schema;
    schema->registerListener(&listener);
    schema->events().disable();
    delete schema;
    ASSERT_IS_NULL(listener.schema)
    ASSERT_LISTENER_NO_EVENTS
}

//------------------------------------------------------------------------------

#define RAISE_EVENT(event, elem)                                             \
    TEST_LOG(#event)                                                         \
    listener.reset();                                                        \
    schema.events().disable();                                               \
    schema.events().raise(SchemaEvents::event, elem);                        \
    ASSERT_IS_TRUE(listener.events.isEmpty())                                \
    schema.events().enable();                                                \
    schema.events().raise(SchemaEvents::event, elem);

TEST_METHOD(raise_all_events)
{
    TestElement e;
    SCHEMA_AND_LISTENER

    RAISE_EVENT(Deleted, nullptr)
    ASSERT_SCHEMA_STATE(STATE(New))
    ASSERT_LISTENER(nullptr, EVENT(Deleted))

    RAISE_EVENT(Changed, nullptr)
    ASSERT_SCHEMA_STATE(STATE(Modified))
    ASSERT_LISTENER(nullptr, EVENT(Changed))

    RAISE_EVENT(Saved, nullptr)
    ASSERT_SCHEMA_STATE(STATE(None))
    ASSERT_LISTENER(nullptr, EVENT(Saved), EVENT(Changed))

    RAISE_EVENT(Loading, nullptr)
    ASSERT_SCHEMA_STATE(STATE(Loading))
    ASSERT_LISTENER(nullptr, EVENT(Loading))

    RAISE_EVENT(Loaded, nullptr)
    ASSERT_SCHEMA_STATE(STATE(None))
    ASSERT_LISTENER(nullptr, EVENT(Loaded), EVENT(Changed))

    RAISE_EVENT(ElemCreated, &e)
    ASSERT_SCHEMA_STATE(STATE(Modified))
    ASSERT_LISTENER(&e, EVENT(ElemCreated), EVENT(Changed))

    SCHEMA_RESET_STATE
    RAISE_EVENT(ElemChanged, &e)
    ASSERT_SCHEMA_STATE(STATE(Modified))
    ASSERT_LISTENER(&e, EVENT(ElemChanged), EVENT(Changed))

    SCHEMA_RESET_STATE
    RAISE_EVENT(ElemDeleting, &e)
    ASSERT_SCHEMA_STATE(STATE(None))
    ASSERT_LISTENER(&e, EVENT(ElemDeleting))

    RAISE_EVENT(ElemDeleted, &e)
    ASSERT_SCHEMA_STATE(STATE(Modified))
    ASSERT_LISTENER(&e, EVENT(ElemDeleted), EVENT(Changed))

    SCHEMA_RESET_STATE
    RAISE_EVENT(ParamsChanged, nullptr)
    ASSERT_SCHEMA_STATE(STATE(Modified))
    ASSERT_LISTENER(nullptr, EVENT(ParamsChanged), EVENT(Changed))

    SCHEMA_RESET_STATE
    RAISE_EVENT(LambdaChanged, nullptr)
    ASSERT_SCHEMA_STATE(STATE(Modified))
    ASSERT_LISTENER(nullptr, EVENT(LambdaChanged), EVENT(Changed))
}

//------------------------------------------------------------------------------

TEST_METHOD(set_wavelength_must_raise_event)
{
    SCHEMA_AND_LISTENER
    schema.wavelength().setValue(Z::Value(10, Z::Units::m()));
    ASSERT_SCHEMA_STATE(STATE(Modified))
    ASSERT_LISTENER(nullptr, EVENT(LambdaChanged), EVENT(Changed))
}

TEST_METHOD(enabledCount)
{
    PREPARE_SCHEMA_ELEMS(10)
    ASSERT_EQ_INT(schema.enabledCount(), 10)
    schema.element(3)->setDisabled(true);
    schema.element(7)->setDisabled(true);
    ASSERT_EQ_INT(schema.enabledCount(), 8)
}

TEST_METHOD(elementById)
{
    PREPARE_SCHEMA_ELEMS(2)
    for (auto e : schema.elements())
        ASSERT_EQ_PTR(schema.elementById(e->id()), e);
}

//------------------------------------------------------------------------------

TEST_METHOD(insertElement_must_increase_count)
{
    Schema schema;
    ASSERT_ELEM_COUNT(0)
    schema.insertElement(new TestElement);
    ASSERT_ELEM_COUNT(1)
}

TEST_METHOD(insertElement_must_assign_element_owner)
{
    Schema schema;
    auto el = new TestElement;
    schema.insertElement(el);
    ASSERT_EQ_PTR(el->owner(), &schema);
}

TEST_METHOD(insertElement_must_raise_events_and_change_state)
{
    SCHEMA_AND_LISTENER
    auto el1 = new TestElement;
    schema.insertElement(el1);
    ASSERT_SCHEMA_STATE(STATE(Modified))
    ASSERT_LISTENER(el1, EVENT(ElemCreated), EVENT(Changed))
}

//------------------------------------------------------------------------------

TEST_METHOD(deleteElement_must_not_fail_when_invalid_elem)
{
    PREPARE_SCHEMA_ELEMS(1)

    // invalid index
    schema.deleteElement(-1);
    ASSERT_ELEM_COUNT(1)

    // invalid index
    schema.deleteElement(1);
    ASSERT_ELEM_COUNT(1)

    // invalid pointer
    TestElement nonSchemaElem;
    schema.deleteElement(&nonSchemaElem);
    ASSERT_ELEM_COUNT(1)
}

TEST_METHOD(deleteElement_must_decrease_count)
{
    PREPARE_SCHEMA_ELEMS(2)
    ASSERT_EQ_INT(schema.count(), 2)

    // by index
    TAKE_ELEM_PTR(el1, 0)
    schema.deleteElement(0, false, false);
    ASSERT_ELEM_COUNT(1)
    ASSERT_EQ_INT(schema.indexOf(el1), -1)

    // by pointer
    TAKE_ELEM_PTR(el2, 0)
    schema.deleteElement(el2, false, false);
    ASSERT_ELEM_COUNT(0)
    ASSERT_EQ_INT(schema.indexOf(el2), -1)
}

TEST_METHOD(deleteElement_must_reset_element_owner)
{
    PREPARE_SCHEMA_ELEMS(2)
    for (auto el: schema.elements())
        ASSERT_EQ_PTR(el->owner(), &schema)

    // by index
    TAKE_ELEM_PTR(el1, 0)
    schema.deleteElement(0, false, false);
    ASSERT_IS_NULL(el1->owner())

    // by pointer
    TAKE_ELEM_PTR(el2, 0)
    schema.deleteElement(el2, false, false);
    ASSERT_IS_NULL(el2->owner())
}

TEST_METHOD(deleteElement_with_lockEvents)
{
    PREPARE_SCHEMA_ELEMS(2)
    schema.events().disable();

    // by index
    TAKE_ELEM_PTR(el1, 0)
    schema.deleteElement(0, true, false);
    ASSERT_LISTENER_NO_EVENTS
    ASSERT_SCHEMA_STATE(STATE(New))

    // by pointer
    TAKE_ELEM_PTR(el2, 0)
    schema.deleteElement(el2, true, false);
    ASSERT_LISTENER_NO_EVENTS
    ASSERT_SCHEMA_STATE(STATE(New))
}

TEST_METHOD(deleteElement_with_events_false)
{
    PREPARE_SCHEMA_ELEMS(2)
    schema.events().enable();

    // by index
    TAKE_ELEM_PTR(el1, 0)
    schema.deleteElement(0, false, false);
    ASSERT_LISTENER_NO_EVENTS
    ASSERT_SCHEMA_STATE(STATE(New))

    // by pointer
    TAKE_ELEM_PTR(el2, 0)
    schema.deleteElement(el2, false, false);
    ASSERT_LISTENER_NO_EVENTS
    ASSERT_SCHEMA_STATE(STATE(New))
}

TEST_METHOD(deleteElement_by_index_with_events)
{
    PREPARE_SCHEMA_ELEMS(1)
    TAKE_ELEM_PTR(el, 0)
    schema.deleteElement(0, true, false);
    ASSERT_SCHEMA_STATE(STATE(Modified))
    ASSERT_LISTENER(el, EVENT(ElemDeleting), EVENT(ElemDeleted), EVENT(Changed))
}

TEST_METHOD(deleteElement_by_pointer_with_events)
{
    PREPARE_SCHEMA_ELEMS(1)
    TAKE_ELEM_PTR(el, 0)
    schema.deleteElement(el, true, false);
    ASSERT_SCHEMA_STATE(STATE(Modified))
    ASSERT_LISTENER(el, EVENT(ElemDeleting), EVENT(ElemDeleted), EVENT(Changed))
}

//------------------------------------------------------------------------------

TEST_GROUP("Schema",
    ADD_TEST(constructor),
    ADD_TEST(destructor_must_raise_event),
    ADD_TEST(destructor_with_no_events),
    ADD_TEST(destructor_must_delete_elements),
    ADD_TEST(raise_all_events),
    ADD_TEST(set_wavelength_must_raise_event),
    ADD_TEST(enabledCount),
    ADD_TEST(elementById),
    ADD_TEST(insertElement_must_increase_count),
    ADD_TEST(insertElement_must_assign_element_owner),
    ADD_TEST(insertElement_must_raise_events_and_change_state),
    ADD_TEST(deleteElement_must_decrease_count),
    ADD_TEST(deleteElement_must_not_fail_when_invalid_elem),
    ADD_TEST(deleteElement_must_reset_element_owner),
    ADD_TEST(deleteElement_with_lockEvents),
    ADD_TEST(deleteElement_with_events_false),
    ADD_TEST(deleteElement_by_index_with_events),
    ADD_TEST(deleteElement_by_pointer_with_events),
)

} // namespace SchemaTests
} // namespace Tests
} // namespace Z