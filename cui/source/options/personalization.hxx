/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_CUI_SOURCE_OPTIONS_PERSONALIZATION_HXX
#define INCLUDED_CUI_SOURCE_OPTIONS_PERSONALIZATION_HXX

#include <sfx2/tabdlg.hxx>
#include <salhelper/thread.hxx>
#include <rtl/ref.hxx>
#include <vcl/prgsbar.hxx>
#include <vector>
#include <array>
#include <atomic>

#define MAX_RESULTS 9           // Maximum number of search results
#define MAX_DEFAULT_PERSONAS 6  // Maximum number of default personas

class FixedText;
class FixedHyperlink;
class SearchAndParseThread;
class GetPersonaThread;

class SvxPersonalizationTabPage : public SfxTabPage
{
    using SfxTabPage::DeactivatePage;

private:
    VclPtr<RadioButton> m_pNoPersona;                  ///< Just the default look, without any bitmap
    VclPtr<RadioButton> m_pDefaultPersona;             ///< Use the built-in bitmap
    VclPtr<RadioButton> m_pOwnPersona;                 ///< Use the user-defined bitmap
    VclPtr<PushButton> m_pSelectPersona;               ///< Let the user select in the 'own' case
    VclPtr<PushButton> m_vDefaultPersonaImages[MAX_DEFAULT_PERSONAS];     ///< Buttons to show the default persona images
    VclPtr<PushButton> m_pExtensionPersonaPreview;     ///< Buttons to show the last 3 personas installed via extensions
    VclPtr<ListBox> m_pPersonaList;                    ///< The ListBox to show the list of installed personas
    OUString m_aPersonaSettings;                       ///< Header and footer images + color to be set in the settings.
    VclPtr<FixedText> m_pExtensionLabel;               ///< The "select persona installed via extensions" label
    VclPtr<FixedHyperlink> m_pAppliedThemeLabel;            ///< The label for showing applied custom theme

    std::vector<OUString> m_vDefaultPersonaSettings;
    std::vector<OUString> m_vExtensionPersonaSettings;

public:
    SvxPersonalizationTabPage( vcl::Window *pParent, const SfxItemSet &rSet );
    virtual ~SvxPersonalizationTabPage() override;
    virtual void dispose() override;

    static VclPtr<SfxTabPage> Create( TabPageParent pParent, const SfxItemSet *rSet );

    /// Apply the settings ([OK] button).
    virtual bool FillItemSet( SfxItemSet *rSet ) override;

    /// Reset to default settings ([Revert] button).
    virtual void Reset( const SfxItemSet *rSet ) override;

    void SetPersonaSettings( const OUString& );
    void CheckAppliedTheme();
    void ShowAppliedThemeLabel( const OUString& );

    /*
     * Loads the default personas from the shared personas directory
     * which resides in the shared gallery.
     * There needs to be a separate subdirectory for each default persona,
     * which includes the preview, header, and footer images.
     * And there needs to be a personas_list.txt file in the personas directory
     * which keeps the index/info of the default personas, one persona per line.
     * A line should look like this:
     * persona_slug;Persona Name;subdir/preview.jpg;subdir/header.jpg;subdir/footer.jpg;#textcolor
     * (It is recommended to keep the subdir name the same as the slug)
     * Example line:
     *  abstract;Abstract;abstract/preview.jpg;abstract/Header2.jpg;abstract/Footer2.jpg;#ffffff
     */
    void LoadDefaultImages();
    void LoadExtensionThemes();

private:
    /// Handle the Persona selection
    DECL_LINK( SelectPersona, Button*, void );

    /// When 'own' is chosen, but the Persona is not chosen yet.
    DECL_LINK( ForceSelect, Button*, void );

    /// Handle the default Persona selection
    DECL_LINK( DefaultPersona, Button*, void );

    /// Handle the Personas installed through extensions selection
    DECL_LINK( SelectInstalledPersona, ListBox&, void );
};

/** Dialog that will allow the user to choose a Persona to use. */
class SelectPersonaDialog : public ModalDialog
{
private:
    VclPtr<Edit> m_pEdit;                                   ///< The input line for the search term
    VclPtr<PushButton> m_pSearchButton;                     ///< The search button
    VclPtr<FixedText> m_pProgressLabel;                     ///< The label for showing progress of search
    VclPtr<PushButton> m_vResultList[MAX_RESULTS];                    ///< List of buttons to show search results
    VclPtr<ListBox> m_pCategories;                         ///< The list of categories
    VclPtr<PushButton> m_pOkButton;                         ///< The OK button
    VclPtr<PushButton> m_pCancelButton;                     ///< The Cancel button

    std::vector<OUString> m_vPersonaSettings;
    OUString m_aSelectedPersona;
    OUString m_aAppliedPersona;

public:
    explicit SelectPersonaDialog( vcl::Window *pParent );
    virtual ~SelectPersonaDialog() override;
    virtual void dispose() override;
    ::rtl::Reference< SearchAndParseThread >    m_pSearchThread;
    ::rtl::Reference< GetPersonaThread >        m_pGetPersonaThread;

    OUString GetSelectedPersona() const;
    void SetProgress( const OUString& );
    /**
     * @brief Assigns preview images to result buttons
     * @param aPreviewImage Persona preview image
     * @param sName Name of the persona
     * @param nIndex Index number of the result button
     */
    void SetImages(const Image& aPreviewImage, const OUString& sName, const sal_Int32& nIndex );
    void AddPersonaSetting( OUString const & );
    void ClearSearchResults();
    void SetAppliedPersonaSetting( OUString const & );
    const OUString& GetAppliedPersonaSetting() const;

private:
    /// Handle the Search button
    DECL_LINK( SearchPersonas, Button*, void );
    /// Handle persona categories list box
    DECL_LINK( SelectCategory, ListBox&, void );
    DECL_LINK( SelectPersona, Button*, void );
    DECL_LINK( ActionOK, Button*, void );
    DECL_LINK( ActionCancel, Button*, void );
};

class SearchAndParseThread: public salhelper::Thread
{
private:

    VclPtr<SelectPersonaDialog> m_pPersonaDialog;
    OUString m_aURL;
    std::atomic<bool> m_bExecute;
    bool m_bDirectURL;

    virtual ~SearchAndParseThread() override;
    virtual void execute() override;

public:

    SearchAndParseThread( SelectPersonaDialog* pDialog,
                          const OUString& rURL, bool bDirectURL );

    void StopExecution() { m_bExecute = false; }
};

class GetPersonaThread: public salhelper::Thread
{
private:

    VclPtr<SelectPersonaDialog> m_pPersonaDialog;
    OUString m_aSelectedPersona;
    std::atomic<bool> m_bExecute;

    virtual ~GetPersonaThread() override;
    virtual void execute() override;

public:

    GetPersonaThread( SelectPersonaDialog* pDialog,
                          const OUString& rSelectedPersona );

    void StopExecution() { m_bExecute = false; }
};

#endif // INCLUDED_CUI_SOURCE_OPTIONS_PERSONALIZATION_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
