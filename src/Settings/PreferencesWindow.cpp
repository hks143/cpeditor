/*
 * Copyright (C) 2019-2020 Ashar Khan <ashar786khan@gmail.com>
 *
 * This file is part of CP Editor.
 *
 * CP Editor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * I will not be responsible if CP Editor behaves in unexpected way and
 * causes your ratings to go down and or lose any important contest.
 *
 * Believe Software is "Software" and it isn't immune to bugs.
 *
 */

#include "Settings/PreferencesWindow.hpp"
#include "Core/EventLogger.hpp"
#include "Settings/AppearancePage.hpp"
#include "Settings/CodeSnippetsPage.hpp"
#include "Settings/DefaultPathManager.hpp"
#include "Settings/ParenthesesPage.hpp"
#include "Settings/PreferencesHomePage.hpp"
#include "Util/Util.hpp"
#include "generated/SettingsHelper.hpp"
#include <QApplication>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScreen>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

AddPageHelper &AddPageHelper::page(const QString &key, const QString &trkey, const QStringList &content)
{
    return page(key, trkey, new PreferencesPageTemplate(content));
}

AddPageHelper &AddPageHelper::page(const QString &key, const QString &trkey, PreferencesPage *newpage)
{
    return page(key, trkey, newpage, newpage->content());
}

AddPageHelper &AddPageHelper::page(const QString &key, const QString &trkey, PreferencesPage *newpage,
                                   const QStringList &content)
{
    window->registerName(key, trkey);
    if (currentPath.size() == 0)
    {
        currentItem = new QTreeWidgetItem({trkey});
        tree->addTopLevelItem(currentItem);
        newpage->setPath(key);
        newpage->setTitle(trkey);
        window->addPage(currentItem, newpage, content);
    }
    else
    {
        QTreeWidgetItem *item = new QTreeWidgetItem({trkey});
        currentItem->addChild(item);
        newpage->setPath(currentPath.join('/') + '/' + key);
        if (key == "@")
            newpage->setTitle(currentItem->text(0));
        else
            newpage->setTitle(trkey);
        window->addPage(item, newpage, content);
    }
    return *this;
}

AddPageHelper &AddPageHelper::dir(const QString &key, const QString &trkey)
{
    window->registerName(key, trkey);
    if (currentPath.size() == 0)
    {
        currentItem = new QTreeWidgetItem({trkey});
        tree->addTopLevelItem(currentItem);
    }
    else
    {
        QTreeWidgetItem *item = new QTreeWidgetItem({trkey});
        currentItem->addChild(item);
        currentItem = item;
    }
    currentPath.push_back(key);
    return *this;
}

AddPageHelper &AddPageHelper::end()
{
    currentItem = currentItem->parent();
    currentPath.pop_back();
    return *this;
}

AddPageHelper::AddPageHelper(PreferencesWindow *w) : window(w), tree(window->menuTree)
{
}

PreferencesWindow::PreferencesWindow(QWidget *parent) : QMainWindow(parent)
{
    // set attributes
    hide();
    setWindowTitle(tr("Preferences"));

    // setup UI
    splitter = new QSplitter();
    splitter->setChildrenCollapsible(false);
    setCentralWidget(splitter);

    leftWidget = new QWidget();
    splitter->addWidget(leftWidget);

    leftLayout = new QVBoxLayout(leftWidget);

    searchLayout = new QHBoxLayout();
    leftLayout->addLayout(searchLayout);

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(tr("Search..."));
    connect(searchEdit, SIGNAL(textChanged(const QString &)), this, SLOT(updateSearch(const QString &)));
    searchLayout->addWidget(searchEdit);

    homeButton = new QPushButton(tr("Home"));
    homeButton->setDisabled(true);
    homeButton->setToolTip(tr("Go to the home page"));
    connect(homeButton, &QPushButton::clicked, this, [this]() { switchToPage(homePage); });
    searchLayout->addWidget(homeButton);

    menuTree = new QTreeWidget();
    menuTree->setHeaderHidden(true);
    connect(menuTree, &QTreeView::clicked, [this](const QModelIndex &index) {
        menuTree->isExpanded(index) ? menuTree->collapse(index) : menuTree->expand(index);
    });
    connect(menuTree, &QTreeWidget::itemActivated, [this](QTreeWidgetItem *item) { switchToPage(pageWidget[item]); });
    connect(menuTree, &QTreeWidget::itemClicked, [this](QTreeWidgetItem *item) { switchToPage(pageWidget[item]); });
    leftLayout->addWidget(menuTree);

    stackedWidget = new QStackedWidget();
    splitter->addWidget(stackedWidget);

    homePage = new PreferencesHomePage();
    connect(homePage, &PreferencesHomePage::requestPage,
            [this](const QString &path) { switchToPage(getPageWidget(path)); });
    stackedWidget->addWidget(homePage);

    splitter->setSizes({10000, 30000});

    exitShortcut = new QShortcut(QKeySequence::Cancel, this);
    connect(exitShortcut, SIGNAL(activated()), this, SLOT(close()));

    travelShortcut = new QShortcut({"Ctrl+Tab"}, this);
    connect(travelShortcut, &QShortcut::activated,
            [this]() { switchToPage(stackedWidget->widget(nextNonHiddenPage(stackedWidget->currentIndex()))); });

    travelBackShortcut = new QShortcut({"Ctrl+Shift+Tab"}, this);
    connect(travelBackShortcut, &QShortcut::activated,
            [this]() { switchToPage(stackedWidget->widget(nextNonHiddenPage(stackedWidget->currentIndex(), -1))); });

    // clang-format off

#define TRKEY(x) x, tr(x)

    AddPageHelper(this)
        .page(TRKEY("Code Edit"),
              {"Tab Width", "Auto Indent", "Wrap Text", "Auto Complete Parentheses", "Auto Remove Parentheses",
               "Tab Jump Out Parentheses", "Replace Tabs", "FakeVim/Enable", "FakeVim/RCLocation"})
        .dir(TRKEY("Language"))
            .page(TRKEY("General"), {"Default Language"})
            .dir(TRKEY("C++"))
                .page("C++ Commands", tr("%1 Commands").arg(tr("C++")),
                      {"C++/Compile Command", "C++/Output Path", "C++/Run Arguments", "C++/Compiler Output Codec"})
                .page("C++ Template", tr("%1 Template").arg(tr("C++")),
                      {"C++/Template Path", "C++/Template Cursor Position Regex",
                       "C++/Template Cursor Position Offset Type", "C++/Template Cursor Position Offset Characters"})
                .page("C++ Snippets", tr("%1 Snippets").arg(tr("C++")), new CodeSnippetsPage("C++"),
                      {"C++ Snippets", "C++ Code Snippets", "Cpp Snippets", "Cpp Code Snippets"})
                .page("C++ Parentheses", tr("%1 Parentheses").arg(tr("C++")), new ParenthesesPage("C++"),
                      {"C++ Parentheses", "C++ Brackets", "C++ Braces", "C++ Auto Complete", "C++ Auto Remove",
                       "C++ Tab Jump Out"})
            .end()
            .dir(TRKEY("Java"))
                .page("Java Commands", tr("%1 Commands").arg(tr("Java")),
                      {"Java/Compile Command", "Java/Output Path", "Java/Class Name", "Java/Run Command", "Java/Run Arguments", "Java/Compiler Output Codec"})
                .page("Java Template", tr("%1 Template").arg(tr("Java")),
                      {"Java/Template Path", "Java/Template Cursor Position Regex", "Java/Template Cursor Position Offset Type",
                       "Java/Template Cursor Position Offset Characters"})
                .page("Java Snippets", tr("%1 Snippets").arg(tr("Java")), new CodeSnippetsPage("Java"),
                      {"Java Snippets", "Java Code Snippets"})
                .page("Java Parentheses", tr("%1 Parentheses").arg(tr("Java")), new ParenthesesPage("Java"),
                      {"Java Parentheses", "Java Brackets", "Java Braces", "Java Auto Complete", "Java Auto Remove",
                       "Java Tab Jump Out"})
            .end()
            .dir(TRKEY("Python"))
                .page("Python Commands", tr("%1 Commands").arg(tr("Python")),
                      {"Python/Run Command", "Python/Run Arguments"})
                .page("Python Template", tr("%1 Template").arg(tr("Python")),
                      {"Python/Template Path", "Python/Template Cursor Position Regex", "Python/Template Cursor Position Offset Type",
                      "Python/Template Cursor Position Offset Characters"})
                .page("Python Snippets", tr("%1 Snippets").arg(tr("Python")), new CodeSnippetsPage("Python"),
                      {"Python Snippets", "Python Code Snippets"})
                .page("Python Parentheses", tr("%1 Parentheses").arg(tr("Python")), new ParenthesesPage("Python"),
                      {"Python Parentheses", "Python Brackets", "Python Braces", "Python Auto Complete", "Python Auto Remove",
                       "Python Tab Jump Out"})
            .end()
        .end()
        .page(TRKEY("Appearance"), new AppearancePage())
        .dir(TRKEY("Actions"))
            .page(TRKEY("Save"), {"Save Faster", "Save File On Compilation", "Save File On Execution", "Save Tests"})
            .page(TRKEY("Auto Save"), {"Auto Save", "Auto Save Interval", "Auto Save Interval Type"})
#ifdef Q_OS_LINUX
            .page(TRKEY("Detached Execution"), {"Detached Run Terminal Program", "Detached Run Terminal Arguments"})
#endif
            .page(TRKEY("Save Session"), {"Hot Exit/Enable", "Hot Exit/Auto Save", "Hot Exit/Auto Save Interval"})
            .page(TRKEY("Bind file and problem"), {"Restore Old Problem Url", "Open Old File For Old Problem Url"})
            .page(TRKEY("Empty Test Cases"), {"Run On Empty Testcase", "Check On Testcases With Empty Output"})
            .page(TRKEY("Load External File Changes"), {"Auto Load External Changes If No Unsaved Modification", "Ask For Loading External Changes"})
        .end()
        .dir(TRKEY("Extensions"))
            .page(TRKEY("Clang Format"), new PreferencesPageTemplate({"Clang Format/Path", "Clang Format/Format On Manual Save",
                                         "Clang Format/Format On Auto Save", "Clang Format/Style"}, false))
	    .dir(TRKEY("Language Server"))
                .page("C++ Server", tr("%1 Server").arg(tr("C++")), {"LSP/Use Linting C++", "LSP/Delay C++", "LSP/Path C++", "LSP/Args C++"})
                .page("Java Server", tr("%1 Server").arg(tr("Java")), {"LSP/Use Linting Java", "LSP/Delay Java", "LSP/Path Java", "LSP/Args Java"})
                .page("Python Server", tr("%1 Server").arg(tr("Python")), {"LSP/Use Linting Python", "LSP/Delay Python", "LSP/Path Python", "LSP/Args Python"})
            .end()
            .page(TRKEY("Competitive Companion"), new PreferencesPageTemplate({"Competitive Companion/Enable",
                "Competitive Companion/Open New Tab", "Competitive Companion/Set Time Limit For Tab",
                "Competitive Companion/Connection Port",
                "Competitive Companion/Head Comments", "Competitive Companion/Head Comments Time Format",
                "Competitive Companion/Head Comments Powered By CP Editor"}, false))
            .page(TRKEY("CF Tool"), {"CF/Path", "CF/Show Toast Messages"})
        .end()
        .dir(TRKEY("File Path"))
            .page(TRKEY("Testcases"), {"Input File Save Path", "Answer File Save Path", "Testcases Matching Rules"})
            .page(TRKEY("Problem URL"), {"Default File Paths For Problem URLs"})
            .page(TRKEY("Default Paths"), DefaultPathManager::actionSettingsList() << "Default Path/Names And Paths")
        .end()
        .page(TRKEY("Key Bindings"), {"Hotkey/Compile", "Hotkey/Run", "Hotkey/Compile Run", "Hotkey/Format", "Hotkey/Kill",
                                   "Hotkey/Change View Mode", "Hotkey/Snippets"})
        .dir(TRKEY("Advanced"))
            .page(TRKEY("Update"), {"Check Update", "Beta"})
            .page(TRKEY("Limits"), {"Default Time Limit", "Output Length Limit", "Output Display Length Limit", "Message Length Limit",
                                    "HTML Diff Viewer Length Limit", "Open File Length Limit", "Load Test Case Length Limit"})
            .page(TRKEY("Network Proxy"), {"Proxy/Enabled", "Proxy/Type", "Proxy/Host Name", "Proxy/Port", "Proxy/User", "Proxy/Password"})
        .end();

#undef TRKEY

    // clang-format on
}

void PreferencesWindow::display()
{
    bool hidden = isHidden();
    Util::showWidgetOnTop(this);
    if (hidden)
    {
        switchToPage(homePage);
        menuTree->clearSelection();
        menuTree->collapseAll();
        searchEdit->clear();
        searchEdit->setFocus();
        move(parentWidget()->geometry().center().x() - width() / 2,
             parentWidget()->geometry().center().y() - height() / 2);
    }
}

void PreferencesWindow::updateSearch(const QString &text)
{
    for (int i = 0; i < menuTree->topLevelItemCount(); ++i)
    {
        updateSearch(menuTree->topLevelItem(i), text);
    }
}

void PreferencesWindow::switchToPage(QWidget *page, bool force)
{
    // return if page is nullptr
    if (page == nullptr)
        return;

    // return if it's no need to switch
    if (stackedWidget->currentWidget() == page)
        return;

    // ask for saving changes or not if not force
    if (!force)
    {
        auto current = qobject_cast<PreferencesPage *>(stackedWidget->currentWidget());
        if (current != nullptr && !current->aboutToExit())
            return;
    }

    // disable home button when it's already at home
    homeButton->setDisabled(page == homePage);

    // switch if everything is OK
    stackedWidget->setCurrentWidget(page);

    auto preferencesPage = qobject_cast<PreferencesPage *>(page);
    if (preferencesPage != nullptr)
    {
        menuTree->setCurrentItem(pageTreeItem[preferencesPage]);
        preferencesPage->loadSettings();
    }
    else
    {
        menuTree->clearSelection();
    }
}

void PreferencesWindow::registerName(const QString &key, const QString &trkey)
{
    if (!treeEntryTranslation.contains(key))
        treeEntryTranslation[key] = trkey;
}

void PreferencesWindow::addPage(QTreeWidgetItem *item, PreferencesPage *page, const QStringList &pageContent)
{
    content[item] = pageContent;
    pageWidget[item] = page;
    pageTreeItem[page] = item;
    stackedWidget->addWidget(page);
    connect(page, SIGNAL(settingsApplied(const QString &)), this, SIGNAL(settingsApplied(const QString &)));
}

PreferencesPage *PreferencesWindow::getPageWidget(const QString &pagePath)
{
    auto parts = pagePath.split('/');
    for (QString &name : parts)
        name = treeEntryTranslation[name];

    QTreeWidgetItem *current = getTopLevelItem(parts.front());

    if (current == nullptr)
        return nullptr;

    // get the non-top-level items step by step
    for (int i = 1; i < parts.count(); ++i)
    {
        QTreeWidgetItem *nxt = getChild(current, parts[i]);
        if (nxt == nullptr)
            return nullptr;
        current = nxt;
    }

    return pageWidget[current];
}

void PreferencesWindow::closeEvent(QCloseEvent *event)
{
    auto current = qobject_cast<PreferencesPage *>(stackedWidget->currentWidget());
    if (!SettingsHelper::isForceClose() && current != nullptr && !current->aboutToExit())
        event->ignore();
    else
        event->accept();
}

void PreferencesWindow::updateSearch(QTreeWidgetItem *item, const QString &text)
{
    if (item == nullptr)
        return;

    item->setExpanded(true);

    if (text.isEmpty())
    {
        // If *text* is empty, set all items visible
        item->setHidden(false);
        for (int i = 0; i < item->childCount(); ++i)
            updateSearch(item->child(i), text);
        return;
    }

    bool shouldHide = true, translatedMatched = false;

    if (item->text(0).contains(text, Qt::CaseInsensitive))
    {
        // check whether the translated path contains *text*
        shouldHide = false;
        translatedMatched = true;
    }
    else if (item->childCount() == 0)
    {
        // check whether the leaf should be hidden or not
        if (pageWidget[item]->path().contains(text, Qt::CaseInsensitive))
        {
            // check whether the untranslated path contains *text*
            shouldHide = false;
        }
        else
        {
            // check whether the content contains *text*
            for (auto s : content[item])
            {
                if (s.contains(text, Qt::CaseInsensitive))
                {
                    shouldHide = false;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < item->childCount(); ++i)
    {
        auto child = item->child(i);

        // show all children if the translation of the current node contains *text*
        updateSearch(child, translatedMatched ? QString() : text);

        // If at least one child is not hidden, a non-leaf item is not hidden
        // this is useful if the untranslated path contains *text*
        if (!child->isHidden())
            shouldHide = false;
    }

    item->setHidden(shouldHide);
}

QTreeWidgetItem *PreferencesWindow::getTopLevelItem(const QString &text)
{
    for (int i = 0; i < menuTree->topLevelItemCount(); ++i)
    {
        auto item = menuTree->topLevelItem(i);
        if (item->text(0) == text)
        {
            return item;
        }
    }
    return nullptr;
}

QTreeWidgetItem *PreferencesWindow::getChild(QTreeWidgetItem *item, const QString &text)
{
    for (int i = 0; i < item->childCount(); ++i)
    {
        auto child = item->child(i);
        if (child->text(0) == text)
        {
            return child;
        }
    }
    return nullptr;
}

int PreferencesWindow::nextNonHiddenPage(int index, int direction, bool includingSelf)
{
    if (index < 0 || index >= stackedWidget->count())
    {
        LOG_WTF("Invalid index: " << index);
        return 0;
    }
    if (direction != 1 && direction != -1)
    {
        LOG_WTF("Invalid direction: " << direction);
        return 0;
    }
    if (includingSelf)
    {
        if (index == 0)
            return 0;
        auto currentWidget = qobject_cast<PreferencesPage *>(stackedWidget->widget(index));
        auto currentItem = pageTreeItem[currentWidget];
        if (currentItem == nullptr)
        {
            LOG_WTF("Failed to get the current item");
            return 0;
        }
        if (!currentItem->isHidden())
            return index;
    }
    return nextNonHiddenPage((index + direction + stackedWidget->count()) % stackedWidget->count(), direction, true);
}
