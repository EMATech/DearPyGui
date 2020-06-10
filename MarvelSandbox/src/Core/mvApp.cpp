#include "mvApp.h"
#include "mvCore.h"
#include "mvLogger.h"
#include "AppItems/mvAppItems.h"

namespace Marvel {

	mvApp* mvApp::s_instance = nullptr;

	mvApp::mvApp()
	{
		m_style = getAppDefaultStyle();
		m_windowflags = ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
	}

	static void SetStyle(ImGuiStyle& style, mvStyle& mvstyle)
	{
		style.Alpha = mvstyle["Alpha"].x;
		style.WindowPadding = { mvstyle["WindowPadding"].x, mvstyle["WindowPadding"].y };
		style.WindowRounding = mvstyle["WindowRounding"].x;
		style.WindowBorderSize = mvstyle["WindowBorderSize"].x;
		style.WindowTitleAlign = { mvstyle["WindowTitleAlign"].x, mvstyle["WindowTitleAlign"].y };
		style.ChildRounding = mvstyle["ChildRounding"].x;
		style.ChildBorderSize = mvstyle["ChildBorderSize"].x;
		style.PopupRounding = mvstyle["PopupRounding"].x;
		style.PopupBorderSize = mvstyle["PopupBorderSize"].x;
		style.FramePadding = { mvstyle["FramePadding"].x, mvstyle["FramePadding"].y };
		style.FrameRounding = mvstyle["FrameRounding"].x;
		style.FrameBorderSize = mvstyle["FrameBorderSize"].x;
		style.ItemSpacing = { mvstyle["ItemSpacing"].x, mvstyle["ItemSpacing"].y };
		style.ItemInnerSpacing = { mvstyle["ItemInnerSpacing"].x, mvstyle["ItemInnerSpacing"].y };
		style.TouchExtraPadding = { mvstyle["TouchExtraPadding"].x, mvstyle["TouchExtraPadding"].y };
		style.IndentSpacing = mvstyle["IndentSpacing"].x;
		style.ScrollbarSize = mvstyle["ScrollbarSize"].x;
		style.ScrollbarRounding = mvstyle["ScrollbarRounding"].x;
		style.GrabMinSize = mvstyle["GrabMinSize"].x;
		style.GrabRounding = mvstyle["GrabRounding"].x;
		style.TabRounding = mvstyle["TabRounding"].x;
		style.TabBorderSize = mvstyle["TabBorderSize"].x;
		style.ButtonTextAlign = { mvstyle["ButtonTextAlign"].x, mvstyle["ButtonTextAlign"].y };
		style.SelectableTextAlign = { mvstyle["SelectableTextAlign"].x, mvstyle["SelectableTextAlign"].y };
		style.DisplayWindowPadding = { mvstyle["DisplayWindowPadding"].x, mvstyle["DisplayWindowPadding"].y };
		style.DisplaySafeAreaPadding = { mvstyle["DisplaySafeAreaPadding"].x, mvstyle["DisplaySafeAreaPadding"].y };
		style.AntiAliasedLines = mvstyle["AntiAliasedLines"].x > 0.5f;
		style.AntiAliasedFill = mvstyle["AntiAliasedFill"].x > 0.5f;
		style.CurveTessellationTol = mvstyle["CurveTessellationTol"].x;
		style.CircleSegmentMaxError = mvstyle["CircleSegmentMaxError"].x;
	}

	static bool doesParentAllowRender(mvAppItem* item)
	{
		if (item->getParent())
		{
			switch (item->getParent()->getType())
			{
			case mvAppItemType::TabItem:
				return static_cast<mvTab*>(item->getParent())->getValue();
				break;

			case mvAppItemType::Menu:
				return static_cast<mvMenu*>(item->getParent())->getValue();
				break;

			case mvAppItemType::Child:
				return static_cast<mvChild*>(item->getParent())->getValue();
				break;

			case mvAppItemType::Tooltip:
				return static_cast<mvTooltip*>(item->getParent())->getValue();
				break;

			case mvAppItemType::Popup:
				return static_cast<mvPopup*>(item->getParent())->getValue();
				break;

			case mvAppItemType::TreeNode:
				return static_cast<mvTreeNode*>(item->getParent())->getValue();
				break;

			case mvAppItemType::CollapsingHeader:
			{
				if (!static_cast<mvCollapsingHeader*>(item->getParent())->getValue())
				{
					item->hide();
					return false;
				}
				else
					item->show();
				break;
			}

			default:
				return item->getParent()->isShown();
			}
		}

		// doesn't have a parent
		return true;
	}

	static void prepareStandardCallbacks()
	{
		ImGuiIO& io = ImGui::GetIO();

		mvApp* app = mvApp::GetApp();

		for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
		{

			if (ImGui::IsMouseClicked(i))
				app->triggerCallback(app->getMouseClickCallback(), std::to_string(i));

			if (io.MouseDownDuration[i] >= 0.0f)
				app->triggerCallback(app->getMouseDownCallback(),
					std::to_string(i), std::to_string(io.MouseDownDuration[i]));

			if (ImGui::IsMouseDoubleClicked(i))
				app->triggerCallback(app->getMouseDoubleClickCallback(), std::to_string(i));

			if (ImGui::IsMouseReleased(i))
				app->triggerCallback(app->getMouseReleaseCallback(), std::to_string(i));
		}

		for (int i = 0; i < IM_ARRAYSIZE(io.KeysDown); i++)
		{
			if (ImGui::IsKeyPressed(i))
				app->triggerCallback(app->getKeyPressCallback(), std::to_string(i));

			if (io.KeysDownDuration[i] >= 0.0f)
				app->triggerCallback(app->getKeyDownCallback(), std::to_string(i), std::to_string(io.KeysDownDuration[i]));

			if (ImGui::IsKeyReleased(i))
				app->triggerCallback(app->getKeyReleaseCallback(), std::to_string(i));
		}
	}

	mvApp* mvApp::GetApp()
	{
		if (s_instance)
			return s_instance;

		s_instance = new mvApp();
		return s_instance;
	}

	void mvApp::render()
	{
		// set imgui style to mvstyle
		updateStyle();

		// update mouse
		ImVec2 mousePos = ImGui::GetMousePos();
		m_mousePos.x = mousePos.x;
		m_mousePos.y = mousePos.y;

		prepareStandardCallbacks();
			
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(m_width, m_height));
		ImGui::Begin("Blah", (bool*)0, m_windowflags);

		m_parents.push(nullptr);

		// main callback
		triggerCallback(m_callback, "Main Application");

		for (mvAppItem* item : m_items)
		{
			// skip item if it's not shown
			if (!item->isShown())
				continue;

			// check if item is owned & it's parent is visible
			if (!doesParentAllowRender(item))
				continue;

			// if parent isn't the most recent parent, skip
			if (item->getParent() && m_parents.top() && item->getType() != mvAppItemType::Tooltip)
				if (!(m_parents.top()->getName() == item->getParent()->getName()))
					continue;

			// set item width
			if(item->getWidth() != 0)
				ImGui::SetNextItemWidth((float)item->getWidth());

			item->draw();

			// Context Menu
			if (item->getPopup() != "")
				ImGui::OpenPopupOnItemClick(item->getPopup().c_str(), getPopupButton(item->getPopup()));

			// Regular Tooltip (simple)
			if (item->getTip() != "" && ImGui::IsItemHovered())
				ImGui::SetTooltip(item->getTip().c_str());

			item->setHovered(ImGui::IsItemHovered());
			item->setActive(ImGui::IsItemActive());
			item->setFocused(ImGui::IsItemFocused());
			item->setClicked(ImGui::IsItemClicked());
			item->setVisible(ImGui::IsItemVisible());
			item->setEdited(ImGui::IsItemEdited());
			item->setActivated(ImGui::IsItemActivated());
			item->setDeactivated(ImGui::IsItemDeactivated());
			item->setDeactivatedAfterEdit(ImGui::IsItemDeactivatedAfterEdit());
			item->setToggledOpen(ImGui::IsItemToggledOpen());
			item->setRectMin({ ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y});
			item->setRectMax({ ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y });
			item->setRectSize({ ImGui::GetItemRectSize().x, ImGui::GetItemRectSize().y });
		}

		ImGui::End();
	}

	bool mvApp::isMouseButtonPressed(int button) const
	{
		ImGuiIO& io = ImGui::GetIO();
		return io.MouseDown[button];
	}

	bool mvApp::isKeyPressed(int keycode) const
	{
		ImGuiIO& io = ImGui::GetIO();
		return io.KeysDown[keycode];
	}

	void mvApp::pushParent(mvAppItem* item)
	{
		m_parents.push(item);
	}

	mvAppItem* mvApp::popParent()
	{
		mvAppItem* item = m_parents.top();
		m_parents.pop();
		return item;
	}

	mvAppItem* mvApp::topParent()
	{
		if(m_parents.size() != 0)
			return m_parents.top();
		return nullptr;
	}

	void mvApp::setItemCallback(const std::string& name, const std::string& callback)
	{
		auto item = getItem(name);
		if (item)
			item->setCallback(callback);
	}

	void mvApp::setItemPopup(const std::string& name, const std::string& popup)
	{
		auto item = getItem(name);
		if (item)
			item->setPopup(popup);
	}

	void mvApp::setItemTip(const std::string& name, const std::string& tip)
	{
		auto item = getItem(name);
		if (item)
			item->setTip(tip);
	}

	void mvApp::setItemWidth(const std::string& name, int width)
	{
		auto item = getItem(name);
		if (item)
			item->setWidth(width);
	}

	mvAppItem* mvApp::getItem(const std::string& name)
	{
		for (mvAppItem* item : m_items)
		{
			if (item->getName() == name)
				return item;
		}

		return nullptr;
	}

	int mvApp::getPopupButton(const std::string& name)
	{
		if (mvAppItem* item = getItem(name))
			return static_cast<mvPopup*>(item)->getButton();
		return -1;
	}

	bool mvApp::isItemHovered(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->isItemHovered();
		return false;
	}

	bool mvApp::isItemActive(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->isItemActive();
		return false;
	}

	bool mvApp::isItemFocused(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->isItemFocused();
		return false;
	}

	bool mvApp::isItemClicked(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->isItemClicked();
		return false;
	}
	
	bool mvApp::isItemVisible(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->isItemVisible();
		return false;
	}
	
	bool mvApp::isItemEdited(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->isItemEdited();
		return false;
	}
	
	bool mvApp::isItemActivated(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->isItemActivated();
		return false;
	}
	
	bool mvApp::isItemDeactivated(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->isItemDeactivated();
		return false;
	}
	
	bool mvApp::isItemDeactivatedAfterEdit(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->isItemDeactivatedAfterEdit();
		return false;
	}
	
	bool mvApp::isItemToogledOpen(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->isItemToogledOpen();
		return false;
	}
	
	mvVec2 mvApp::getItemRectMin(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->getItemRectMin();
		return {0.0f, 0.0f};
	}
	
	mvVec2 mvApp::getItemRectMax(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->getItemRectMax();
		return { 0.0f, 0.0f };
	}
	
	mvVec2 mvApp::getItemRectSize(const std::string& item)
	{
		if (mvAppItem* pitem = getItem(item))
			return pitem->getItemRectSize();
		return { 0.0f, 0.0f };
	}

	void mvApp::triggerCallback(const std::string& name, const std::string& sender)
	{
		if (name == "")
			return;

		PyErr_Clear();

		PyObject* pHandler = PyDict_GetItemString(m_pDict, name.c_str()); // borrowed reference

		PyObject* pArgs = PyTuple_New(1);
		PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(sender.c_str()));

		PyObject* result = PyObject_CallObject(pHandler, pArgs);

		// check if error occurred
		PyErr_Print();


		Py_XDECREF(pArgs);
		Py_XDECREF(result);

	}

	void mvApp::triggerCallback(const std::string& name, const std::string& sender, const std::string& data)
	{
		if (name == "")
			return;

		PyErr_Clear();

		PyObject* pHandler = PyDict_GetItemString(m_pDict, name.c_str()); // borrowed reference

		PyObject* pArgs = PyTuple_New(2);
		PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(sender.c_str()));
		PyTuple_SetItem(pArgs, 1, PyUnicode_FromString(data.c_str()));

		PyObject* result = PyObject_CallObject(pHandler, pArgs);

		// check if error occurred
		PyErr_Print();


		Py_XDECREF(pArgs);
		Py_XDECREF(result);

	}

	void mvApp::setAppTheme(const std::string& theme)
	{
		if (theme == "dark")
			ImGui::StyleColorsDark();
		else if (theme == "classic")
			ImGui::StyleColorsClassic();
		else if (theme == "light")
			ImGui::StyleColorsLight();
		else
			ImGui::StyleColorsDark();
	}

	void mvApp::updateTheme()
	{
		ImGuiStyle* style = &ImGui::GetStyle();
		ImVec4* colors = style->Colors;

		auto themecolors = m_theme.getColors();

		for (unsigned i = 0; i < m_theme.getNumberOfItems(); i++)
		{
			auto color = themecolors[i];
			colors[i] = ImVec4(color[0], color[1], color[2], color[3]);
		}
			
	}

	void mvApp::changeThemeItem(const char* name, mvColor color)
	{
		m_theme.changeThemeItem(name, color.r, color.g, color.b, color.a);
	}

	void mvApp::updateStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		SetStyle(style, m_style);
	}

	bool mvApp::setStyleItem(const std::string& item, float x, float y)
	{

		if (m_style.count(item) > 0)
		{
			m_style.at(item).x = x;
			m_style.at(item).y = y;
			return true;
		}

		return false;
	}

	void mvApp::addItemManual(mvAppItem* item)
	{
		m_items.push_back(item);
	}

	void mvApp::addItem(mvAppItem* item)
	{
		mvAppItem* parentitem = topParent();	
		item->setParent(parentitem);
		m_items.push_back(item);
	}

	void mvApp::addParentItem(mvAppItem* item)
	{
		addItem(item);
		pushParent(item);
	}

	void mvApp::addEndParentItem(mvAppItem* item)
	{
		addItem(item);
		popParent();
	}

	void mvApp::closePopup()
	{
		ImGui::CloseCurrentPopup();
	}

	void mvApp::Log(const std::string& text, const std::string& level)
	{
		if (m_loglevel < 1)
			AppLog::getLogger()->AddLog("[%05d] [%1s]  %2s\n", ImGui::GetFrameCount(), level.c_str(), text.c_str());
	}

	void mvApp::LogDebug(const std::string& text)
	{
		if (m_loglevel < 2)
			AppLog::getLogger()->AddLog("[%05d] [DEBUG]  %1s\n", ImGui::GetFrameCount(), text.c_str());
	}

	void mvApp::LogInfo(const std::string& text)
	{
		if (m_loglevel < 3)
			AppLog::getLogger()->AddLog("[%05d] [INFO]  %1s\n", ImGui::GetFrameCount(), text.c_str());
	}

	void mvApp::LogWarning(const std::string& text)
	{
		if (m_loglevel < 4)
			AppLog::getLogger()->AddLog("[%05d] [WARNING]  %1s\n", ImGui::GetFrameCount(), text.c_str());
	}

	void mvApp::LogError(const std::string& text)
	{
		if (m_loglevel < 5)
			AppLog::getLogger()->AddLog("[%05d] [ERROR]  %1s\n", ImGui::GetFrameCount(), text.c_str());
	}

	void mvApp::ClearLog()
	{
		AppLog::getLogger()->Clear();
	}

}