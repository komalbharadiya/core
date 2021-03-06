/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <swmodeltestbase.hxx>
#include <com/sun/star/awt/FontSlant.hpp>
#include <com/sun/star/text/TextContentAnchorType.hpp>
#include <com/sun/star/text/AutoTextContainer.hpp>
#include <com/sun/star/text/XAutoTextGroup.hpp>
#include <com/sun/star/rdf/URI.hpp>
#include <com/sun/star/rdf/URIs.hpp>
#include <wrtsh.hxx>
#include <ndtxt.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::text;

namespace
{
char const DATA_DIRECTORY[] = "/sw/qa/extras/unowriter/data/";
}

/// Test to assert UNO API call results of Writer.
class SwUnoWriter : public SwModelTestBase
{
public:
    SwUnoWriter()
        : SwModelTestBase(DATA_DIRECTORY, "writer8")
    {
    }
};

/**
 * Macro to declare a new test with preloaded file
 * (similar to DECLARE_SW_ROUNDTRIP_TEST)
 */
#define DECLARE_UNOAPI_TEST_FILE(TestName, filename)                                               \
    class TestName : public SwUnoWriter                                                            \
    {                                                                                              \
    protected:                                                                                     \
        virtual OUString getTestName() override { return OUString(#TestName); }                    \
                                                                                                   \
    public:                                                                                        \
        CPPUNIT_TEST_SUITE(TestName);                                                              \
        CPPUNIT_TEST(loadAndTest);                                                                 \
        CPPUNIT_TEST_SUITE_END();                                                                  \
        void loadAndTest()                                                                         \
        {                                                                                          \
            load(mpTestDocumentPath, filename);                                                    \
            runTest();                                                                             \
        }                                                                                          \
        void runTest();                                                                            \
    };                                                                                             \
    CPPUNIT_TEST_SUITE_REGISTRATION(TestName);                                                     \
    void TestName::runTest()

/**
 * Macro to declare a new test without loading any files
 */
#define DECLARE_UNOAPI_TEST(TestName)                                                              \
    class TestName : public SwUnoWriter                                                            \
    {                                                                                              \
    protected:                                                                                     \
        virtual OUString getTestName() override { return OUString(#TestName); }                    \
                                                                                                   \
    public:                                                                                        \
        CPPUNIT_TEST_SUITE(TestName);                                                              \
        CPPUNIT_TEST(runTest);                                                                     \
        CPPUNIT_TEST_SUITE_END();                                                                  \
        void runTest();                                                                            \
    };                                                                                             \
    CPPUNIT_TEST_SUITE_REGISTRATION(TestName);                                                     \
    void TestName::runTest()

DECLARE_UNOAPI_TEST(testDefaultCharStyle)
{
    // Create a new document, type a character, set its char style to Emphasis
    // and assert the style was set.
    loadURL("private:factory/swriter", nullptr);

    uno::Reference<text::XTextDocument> xTextDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XSimpleText> xBodyText(xTextDocument->getText(), uno::UNO_QUERY);
    xBodyText->insertString(xBodyText->getStart(), "x", false);

    uno::Reference<text::XTextCursor> xCursor(xBodyText->createTextCursor());
    xCursor->goLeft(1, true);

    uno::Reference<beans::XPropertySet> xCursorProps(xCursor, uno::UNO_QUERY);
    xCursorProps->setPropertyValue("CharStyleName", uno::makeAny(OUString("Emphasis")));
    CPPUNIT_ASSERT_EQUAL(awt::FontSlant_ITALIC,
                         getProperty<awt::FontSlant>(xCursorProps, "CharPosture"));

    // Now reset the char style and assert that the font slant is back to none.
    // This resulted in a lang.IllegalArgumentException, Standard was not
    // mapped to 'Default Style'.
    xCursorProps->setPropertyValue("CharStyleName", uno::makeAny(OUString("Standard")));
    CPPUNIT_ASSERT_EQUAL(awt::FontSlant_NONE,
                         getProperty<awt::FontSlant>(xCursorProps, "CharPosture"));
}

DECLARE_UNOAPI_TEST(testGraphicDesciptorURL)
{
    loadURL("private:factory/swriter", nullptr);

    // Create a graphic object, but don't insert it yet.
    uno::Reference<lang::XMultiServiceFactory> xFactory(mxComponent, uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xTextGraphic(
        xFactory->createInstance("com.sun.star.text.TextGraphicObject"), uno::UNO_QUERY);

    // Set an URL on it.
    OUString aGraphicURL = m_directories.getURLFromSrc(DATA_DIRECTORY) + "test.jpg";
    xTextGraphic->setPropertyValue("GraphicURL", uno::makeAny(aGraphicURL));
    xTextGraphic->setPropertyValue("AnchorType",
                                   uno::makeAny(text::TextContentAnchorType_AT_CHARACTER));

    // Insert it.
    uno::Reference<text::XTextDocument> xTextDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XText> xBodyText(xTextDocument->getText(), uno::UNO_QUERY);
    uno::Reference<text::XTextCursor> xCursor(xBodyText->createTextCursor());
    uno::Reference<text::XTextContent> xTextContent(xTextGraphic, uno::UNO_QUERY);
    xBodyText->insertTextContent(xCursor, xTextContent, false);

    // This failed, the graphic object had no graphic.
    auto xGraphic = getProperty<uno::Reference<graphic::XGraphic>>(getShape(1), "Graphic");
    CPPUNIT_ASSERT(xGraphic.is());
}

DECLARE_UNOAPI_TEST(testGraphicDesciptorURLBitmap)
{
    loadURL("private:factory/swriter", nullptr);

    // Load a bitmap into the bitmap table.
    uno::Reference<lang::XMultiServiceFactory> xFactory(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XNameContainer> xBitmaps(
        xFactory->createInstance("com.sun.star.drawing.BitmapTable"), uno::UNO_QUERY);
    OUString aGraphicURL = m_directories.getURLFromSrc(DATA_DIRECTORY) + "test.jpg";
    xBitmaps->insertByName("test", uno::makeAny(aGraphicURL));

    // Create a graphic.
    uno::Reference<beans::XPropertySet> xTextGraphic(
        xFactory->createInstance("com.sun.star.text.TextGraphicObject"), uno::UNO_QUERY);
    xTextGraphic->setPropertyValue("GraphicURL", xBitmaps->getByName("test"));
    xTextGraphic->setPropertyValue("AnchorType",
                                   uno::makeAny(text::TextContentAnchorType_AT_CHARACTER));

    // Insert it.
    uno::Reference<text::XTextDocument> xTextDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XText> xBodyText(xTextDocument->getText(), uno::UNO_QUERY);
    uno::Reference<text::XTextCursor> xCursor(xBodyText->createTextCursor());
    uno::Reference<text::XTextContent> xTextContent(xTextGraphic, uno::UNO_QUERY);
    xBodyText->insertTextContent(xCursor, xTextContent, false);

    // This failed: setting GraphicURL to the result of getByName() did not
    // work anymore.
    auto xGraphic = getProperty<uno::Reference<graphic::XGraphic>>(getShape(1), "Graphic");
    CPPUNIT_ASSERT(xGraphic.is());
}

static bool ensureAutoTextExistsByTitle(const Reference<XAutoTextGroup>& autoTextGroup,
                                        const OUString& autoTextName)
{
    uno::Sequence<OUString> aTitles(autoTextGroup->getTitles());
    for (const auto& rTitle : aTitles)
    {
        if (rTitle == autoTextName)
            return true;
    }
    return false;
}

static bool ensureAutoTextExistsByName(const Reference<XAutoTextGroup>& autoTextGroup,
                                       const OUString& autoTextName)
{
    uno::Sequence<OUString> aTitles(autoTextGroup->getElementNames());
    for (const auto& rTitle : aTitles)
    {
        if (rTitle == autoTextName)
            return true;
    }
    return false;
}

DECLARE_UNOAPI_TEST_FILE(testXAutoTextGroup, "xautotextgroup.odt")
{
    Reference<XAutoTextContainer> xAutoTextContainer
        = AutoTextContainer::create(comphelper::getProcessComponentContext());

    uno::Reference<text::XTextRange> xTextRange = getRun(getParagraph(1), 1);

    const OUString sGroupName = "TestGroup*1";
    const OUString sTextName = "TEST";
    const OUString sTextNameNew = "TESTRENAMED";
    const OUString sTextTitle = "Test Auto Text";
    const OUString sTextTitleNew = "Test Auto Text Renamed";

    // Create new temporary group
    Reference<XAutoTextGroup> xAutoTextGroup(xAutoTextContainer->insertNewByName(sGroupName),
                                             uno::UNO_QUERY);
    CPPUNIT_ASSERT_MESSAGE("AutoTextGroup was not found!", xAutoTextGroup.is());

    // Insert new element and ensure it exists
    Reference<XAutoTextEntry> xAutoTextEntry
        = xAutoTextGroup->insertNewByName(sTextName, sTextTitle, xTextRange);
    CPPUNIT_ASSERT_MESSAGE("AutoText was not inserted!", xAutoTextEntry.is());
    CPPUNIT_ASSERT_MESSAGE("Can't find newly created AutoText by title!",
                           ensureAutoTextExistsByTitle(xAutoTextGroup, sTextTitle));
    CPPUNIT_ASSERT_MESSAGE("Can't find newly created AutoText by name!",
                           ensureAutoTextExistsByName(xAutoTextGroup, sTextName));

    // Insert once again the same should throw an exception
    CPPUNIT_ASSERT_THROW_MESSAGE("We expect an exception on insertion of same AutoText",
                                 xAutoTextGroup->insertNewByName(sTextName, sTextTitle, xTextRange),
                                 container::ElementExistException);

    // Rename it & ensure everything is ok
    xAutoTextGroup->renameByName(sTextName, sTextNameNew, sTextTitleNew);
    CPPUNIT_ASSERT_MESSAGE("Can't find renamed AutoText by title!",
                           ensureAutoTextExistsByTitle(xAutoTextGroup, sTextTitleNew));
    CPPUNIT_ASSERT_MESSAGE("Can't find renamed AutoText by name!",
                           ensureAutoTextExistsByName(xAutoTextGroup, sTextNameNew));
    // Not found by old names
    CPPUNIT_ASSERT_MESSAGE("Found AutoText by old title!",
                           !ensureAutoTextExistsByTitle(xAutoTextGroup, sTextTitle));
    CPPUNIT_ASSERT_MESSAGE("Found AutoText by old name!",
                           !ensureAutoTextExistsByName(xAutoTextGroup, sTextName));

    // Rename not existing should throw an exception
    CPPUNIT_ASSERT_THROW_MESSAGE(
        "We expect an exception on renaming not-exising AutoText",
        xAutoTextGroup->renameByName(sTextName, sTextNameNew, sTextTitleNew),
        container::ElementExistException);

    // Remove it and ensure it does not exist
    xAutoTextGroup->removeByName(sTextNameNew);
    CPPUNIT_ASSERT_MESSAGE("AutoText was not removed!",
                           !ensureAutoTextExistsByTitle(xAutoTextGroup, sTextTitleNew));
    CPPUNIT_ASSERT_MESSAGE("AutoText was not removed!",
                           !ensureAutoTextExistsByName(xAutoTextGroup, sTextNameNew));

    // Remove non-existing element should throw an exception
    CPPUNIT_ASSERT_THROW_MESSAGE("We expect an exception on removing not-existing AutoText",
                                 xAutoTextGroup->removeByName(sTextName),
                                 container::NoSuchElementException);

    // Remove our temporary group
    xAutoTextContainer->removeByName(sGroupName);
}

DECLARE_UNOAPI_TEST(testXURI)
{
    Reference<XComponentContext> xContext(::comphelper::getProcessComponentContext());

    // createKnown()
    Reference<rdf::XURI> xURIcreateKnown(rdf::URI::createKnown(xContext, rdf::URIs::ODF_PREFIX),
                                         UNO_SET_THROW);
    CPPUNIT_ASSERT(xURIcreateKnown.is());
    CPPUNIT_ASSERT_EQUAL(OUString("http://docs.oasis-open.org/ns/office/1.2/meta/odf#"),
                         xURIcreateKnown->getNamespace());
    CPPUNIT_ASSERT_EQUAL(OUString("prefix"), xURIcreateKnown->getLocalName());
    CPPUNIT_ASSERT_EQUAL(OUString("http://docs.oasis-open.org/ns/office/1.2/meta/odf#prefix"),
                         xURIcreateKnown->getStringValue());

    // createKnown() with invalid constant
    CPPUNIT_ASSERT_THROW_MESSAGE("We expect an exception on invalid constant",
                                 rdf::URI::createKnown(xContext, 12345),
                                 lang::IllegalArgumentException);

    // create()
    Reference<rdf::XURI> xURIcreate(rdf::URI::create(xContext, "http://example.com/url#somedata"),
                                    UNO_SET_THROW);
    CPPUNIT_ASSERT_EQUAL(OUString("http://example.com/url#"), xURIcreate->getNamespace());
    CPPUNIT_ASSERT_EQUAL(OUString("somedata"), xURIcreate->getLocalName());
    CPPUNIT_ASSERT_EQUAL(OUString("http://example.com/url#somedata"), xURIcreate->getStringValue());

    // create() without local name splitted with "/"
    Reference<rdf::XURI> xURIcreate2(rdf::URI::create(xContext, "http://example.com/url"),
                                     UNO_SET_THROW);
    CPPUNIT_ASSERT_EQUAL(OUString("http://example.com/"), xURIcreate2->getNamespace());
    CPPUNIT_ASSERT_EQUAL(OUString("url"), xURIcreate2->getLocalName());
    CPPUNIT_ASSERT_EQUAL(OUString("http://example.com/url"), xURIcreate2->getStringValue());

    // create() without prefix
    Reference<rdf::XURI> xURIcreate3(rdf::URI::create(xContext, "#somedata"), UNO_SET_THROW);
    CPPUNIT_ASSERT_EQUAL(OUString("#"), xURIcreate3->getNamespace());
    CPPUNIT_ASSERT_EQUAL(OUString("somedata"), xURIcreate3->getLocalName());
    CPPUNIT_ASSERT_EQUAL(OUString("#somedata"), xURIcreate3->getStringValue());

    // create() with invalid URI
    CPPUNIT_ASSERT_THROW_MESSAGE("We expect an exception on invalid URI",
                                 rdf::URI::create(xContext, "some junk and not URI"),
                                 lang::IllegalArgumentException);

    // createNS()
    Reference<rdf::XURI> xURIcreateNS(
        rdf::URI::createNS(xContext, "http://example.com/url#", "somedata"), UNO_SET_THROW);
    CPPUNIT_ASSERT_EQUAL(OUString("http://example.com/url#"), xURIcreateNS->getNamespace());
    CPPUNIT_ASSERT_EQUAL(OUString("somedata"), xURIcreateNS->getLocalName());
    CPPUNIT_ASSERT_EQUAL(OUString("http://example.com/url#somedata"),
                         xURIcreateNS->getStringValue());

    // TODO: What's going on here? Is such usecase valid?
    Reference<rdf::XURI> xURIcreateNS2(
        rdf::URI::createNS(xContext, "http://example.com/url", "somedata"), UNO_SET_THROW);
    CPPUNIT_ASSERT_EQUAL(OUString("http://example.com/"), xURIcreateNS2->getNamespace());
    CPPUNIT_ASSERT_EQUAL(OUString("urlsomedata"), xURIcreateNS2->getLocalName());
    CPPUNIT_ASSERT_EQUAL(OUString("http://example.com/urlsomedata"),
                         xURIcreateNS2->getStringValue());

    // createNS() some invalid cases
    CPPUNIT_ASSERT_THROW_MESSAGE("We expect an exception on invalid URI",
                                 rdf::URI::createNS(xContext, "bla", "bla"),
                                 lang::IllegalArgumentException);

    CPPUNIT_ASSERT_THROW_MESSAGE("We expect an exception on invalid URI",
                                 rdf::URI::createNS(xContext, OUString(), OUString()),
                                 lang::IllegalArgumentException);
}

DECLARE_UNOAPI_TEST(testSetPagePrintSettings)
{
    // Create an empty new document with a single char
    loadURL("private:factory/swriter", nullptr);

    uno::Reference<text::XTextDocument> xTextDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<text::XSimpleText> xBodyText(xTextDocument->getText(), uno::UNO_QUERY);
    xBodyText->insertString(xBodyText->getStart(), "x", false);

    uno::Reference<text::XPagePrintable> xPagePrintable(mxComponent, uno::UNO_QUERY);

    // set some stuff, try to get it back
    uno::Sequence<beans::PropertyValue> aProps(2);
    aProps[0].Name = "PageColumns";
    aProps[0].Value <<= sal_Int16(2);
    aProps[1].Name = "IsLandscape";
    aProps[1].Value <<= true;

    xPagePrintable->setPagePrintSettings(aProps);
    const comphelper::SequenceAsHashMap aMap(xPagePrintable->getPagePrintSettings());

    CPPUNIT_ASSERT_EQUAL(sal_Int16(2), aMap.getValue("PageColumns").get<short>());
    CPPUNIT_ASSERT_EQUAL(true, aMap.getValue("IsLandscape").get<bool>());
}

DECLARE_UNOAPI_TEST_FILE(testSelectionInTableEnum, "selection-in-table-enum.odt")
{
    // Select the A1 cell's text.
    SwXTextDocument* pTextDoc = dynamic_cast<SwXTextDocument*>(mxComponent.get());
    CPPUNIT_ASSERT(pTextDoc);
    SwWrtShell* pWrtShell = pTextDoc->GetDocShell()->GetWrtShell();
    CPPUNIT_ASSERT(pWrtShell);
    pWrtShell->Down(/*bSelect=*/false);
    pWrtShell->EndPara(/*bSelect=*/true);
    CPPUNIT_ASSERT_EQUAL(OUString("A1"),
                         pWrtShell->GetCursor()->GetNode().GetTextNode()->GetText());

    // Access the selection.
    uno::Reference<frame::XModel> xModel(mxComponent, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xModel.is());
    uno::Reference<container::XIndexAccess> xSelections(xModel->getCurrentSelection(),
                                                        uno::UNO_QUERY);
    CPPUNIT_ASSERT(xSelections.is());
    uno::Reference<text::XTextRange> xSelection(xSelections->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xSelection.is());

    // Enumerate paragraphs in the selection.
    uno::Reference<container::XEnumerationAccess> xCursor(
        xSelection->getText()->createTextCursorByRange(xSelection), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xCursor.is());
    uno::Reference<container::XEnumeration> xEnum = xCursor->createEnumeration();
    xEnum->nextElement();
    // Without the accompanying fix in place, this test would have failed: i.e.
    // the enumeration contained a second paragraph, even if the cell has only
    // one paragraph.
    CPPUNIT_ASSERT(!xEnum->hasMoreElements());
}

DECLARE_UNOAPI_TEST_FILE(testSelectionInTableEnumEnd, "selection-in-table-enum.odt")
{
    // Select from "Before" till the table end.
    SwXTextDocument* pTextDoc = dynamic_cast<SwXTextDocument*>(mxComponent.get());
    CPPUNIT_ASSERT(pTextDoc);
    SwWrtShell* pWrtShell = pTextDoc->GetDocShell()->GetWrtShell();
    CPPUNIT_ASSERT(pWrtShell);
    pWrtShell->Down(/*bSelect=*/true);

    // Access the selection.
    uno::Reference<frame::XModel> xModel(mxComponent, uno::UNO_QUERY);
    CPPUNIT_ASSERT(xModel.is());
    uno::Reference<container::XIndexAccess> xSelections(xModel->getCurrentSelection(),
                                                        uno::UNO_QUERY);
    CPPUNIT_ASSERT(xSelections.is());
    uno::Reference<text::XTextRange> xSelection(xSelections->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xSelection.is());
    OUString aExpectedSelection
        = "Before" SAL_NEWLINE_STRING "A1" SAL_NEWLINE_STRING "B1" SAL_NEWLINE_STRING
          "C2" SAL_NEWLINE_STRING "A2" SAL_NEWLINE_STRING "B2" SAL_NEWLINE_STRING
          "C2" SAL_NEWLINE_STRING;
    CPPUNIT_ASSERT_EQUAL(aExpectedSelection, xSelection->getString());

    // Enumerate paragraphs in the selection.
    uno::Reference<container::XEnumerationAccess> xCursor(
        xSelection->getText()->createTextCursorByRange(xSelection), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xCursor.is());
    uno::Reference<container::XEnumeration> xEnum = xCursor->createEnumeration();
    // Before.
    xEnum->nextElement();
    // Table.
    xEnum->nextElement();
    // Without the accompanying fix in place, this test would have failed: i.e.
    // the enumeration contained the paragraph after the table, but no part of
    // that paragraph was part of the selection.
    CPPUNIT_ASSERT(!xEnum->hasMoreElements());
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
