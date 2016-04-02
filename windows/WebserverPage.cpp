/*
* Copyright (C) 2011-2016 AirDC++ Project
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#include "stdafx.h"
#include "Resource.h"

#include "WebServerPage.h"
#include "WebUserDlg.h"

#include <airdcpp/LogManager.h>


PropPage::TextItem WebServerPage::texts[] = {
	{ IDC_WEBSERVER_ADD_USER,					ResourceManager::ADD },
	{ IDC_WEBSERVER_REMOVE_USER,				ResourceManager::REMOVE },
	{ IDC_WEBSERVER_CHANGE,						ResourceManager::SETTINGS_CHANGE },

	{ IDC_ADMIN_ACCOUNTS,						ResourceManager::ADMIN_ACCOUNTS },
	{ IDC_WEBSERVER_LABEL,						ResourceManager::SERVER_SETTINGS },
	{ IDC_WEB_SERVER_USERS_NOTE,				ResourceManager::WEB_ACCOUNTS_NOTE },

	{ IDC_WEB_SERVER_HELP,						ResourceManager::WEB_SERVER_HELP },
	{ IDC_LINK,									ResourceManager::MORE_INFORMATION },
	{ 0, ResourceManager::LAST }
};

WebServerPage::WebServerPage(SettingsManager *s) : PropPage(s), webMgr(webserver::WebServerManager::getInstance()) {
	SetTitle(CTSTRING(WEB_SERVER));
};

LRESULT WebServerPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::translate((HWND)(*this), texts);

	::SetWindowText(GetDlgItem(IDC_WEBSERVER_PORT_LABEL), (TSTRING(PORT) + _T(" (HTTP)")).c_str());
	::SetWindowText(GetDlgItem(IDC_WEBSERVER_TLSPORT_LABEL), (TSTRING(PORT) + _T(" (HTTPS)")).c_str());

	::SetWindowText(GetDlgItem(IDC_WEBSERVER_PORT), Util::toStringW(webMgr->getPlainServerConfig().getPort()).c_str());
	::SetWindowText(GetDlgItem(IDC_WEBSERVER_TLSPORT), Util::toStringW(webMgr->getTlsServerConfig().getPort()).c_str());

	ctrlTlsPort.Attach(GetDlgItem(IDC_WEBSERVER_TLSPORT));
	ctrlPort.Attach(GetDlgItem(IDC_WEBSERVER_PORT));

	url.SubclassWindow(GetDlgItem(IDC_LINK));
	url.SetHyperLinkExtendedStyle(HLINK_UNDERLINEHOVER);

	// TODO: add better help link
	url.SetHyperLink(_T("http://www.airdcpp.net/forum/viewtopic.php?t=4334"));
	url.SetLabel(CTSTRING(MORE_INFORMATION));

	ctrlWebUsers.Attach(GetDlgItem(IDC_WEBSERVER_USERS));
	ctrlWebUsers.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);

	CRect rc;
	ctrlWebUsers.GetClientRect(rc);
	ctrlWebUsers.InsertColumn(0, CTSTRING(NAME), LVCFMT_LEFT, rc.Width() / 2, 0);
	ctrlWebUsers.InsertColumn(1, CTSTRING(PASSWORD), LVCFMT_LEFT, rc.Width() / 2, 1);

	ctrlRemove.Attach(GetDlgItem(IDC_WEBSERVER_REMOVE_USER));
	ctrlAdd.Attach(GetDlgItem(IDC_WEBSERVER_ADD_USER));
	ctrlChange.Attach(GetDlgItem(IDC_WEBSERVER_CHANGE));
	ctrlStart.Attach(GetDlgItem(IDC_WEBSERVER_START));
	ctrlStatus.Attach(GetDlgItem(IDC_WEBSERVER_STATUS));

	updateState(webMgr->isRunning() ? STATE_STARTED : STATE_STOPPED);

	webUserList = webMgr->getUserManager().getUsers();
	for (const auto& u : webUserList) {
		if (!u->isAdmin()) {
			continue;
		}

		addListItem(u->getUserName(), u->getPassword());
	}

	webMgr->addListener(this);
	return TRUE;
}

void WebServerPage::applySettings() noexcept {
	auto plainserverPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlPort)));
	auto tlsServerPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlTlsPort)));

	webMgr->getPlainServerConfig().setPort(plainserverPort);
	webMgr->getTlsServerConfig().setPort(tlsServerPort);

	webMgr->getUserManager().replaceWebUsers(webUserList);
}

LRESULT WebServerPage::onServerState(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!webMgr->isRunning()) {
		ctrlStart.EnableWindow(FALSE);
		lastError.clear();

		applySettings();

		bool started = webMgr->start([this](const string& aError) { 
			setLastError(aError);
		});

		if (!started) {
			// There can be endpoint-specific listening errors already
			if (lastError.empty()) {
				setLastError(STRING(WEB_SERVER_INVALID_CONFIG));
			}
			
			updateState(STATE_STOPPED);
		}
	} else {
		lastError.clear();
		webserver::WebServerManager::getInstance()->stop();
	}

	return 0;
}

void WebServerPage::on(webserver::WebServerManagerListener::Started) noexcept {
	callAsync([=] {
		updateState(STATE_STARTED);
	});
}
void WebServerPage::on(webserver::WebServerManagerListener::Stopped) noexcept {
	callAsync([=] {
		updateState(STATE_STOPPED);
	});
}

void WebServerPage::on(webserver::WebServerManagerListener::Stopping) noexcept {
	callAsync([=] {
		updateState(STATE_STOPPING);
	});
}

LRESULT WebServerPage::onChangeUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (ctrlWebUsers.GetSelectedCount() == 1) {
		int sel = ctrlWebUsers.GetSelectedIndex();
		auto webUser = webUserList[sel];
		WebUserDlg dlg(Text::toT(webUser->getUserName()), Text::toT(webUser->getPassword()));
		if (dlg.DoModal() == IDOK) {
			if (!dlg.getUserName().empty()) webUser->setUserName(dlg.getUserName());
			if (!dlg.getPassWord().empty()) webUser->setPassword(dlg.getPassWord());
			ctrlWebUsers.SetItemText(sel, 0, Text::toT(webUser->getUserName()).c_str());
			ctrlWebUsers.SetItemText(sel, 1, Text::toT(webUser->getPassword()).c_str());
		}
	}

	return 0;
}

LRESULT WebServerPage::onAddUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	WebUserDlg dlg;
	if (dlg.DoModal() == IDOK) {
		if (!dlg.getUserName().empty() && !dlg.getPassWord().empty()) {
			webUserList.emplace_back(make_shared<webserver::WebUser>(dlg.getUserName(), dlg.getPassWord()));
			addListItem(dlg.getUserName(), dlg.getPassWord());
		}
	}

	return 0;
}

LRESULT WebServerPage::onRemoveUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (ctrlWebUsers.GetSelectedCount() == 1) {
		int sel = ctrlWebUsers.GetSelectedIndex();
		webUserList.erase(find(webUserList.begin(), webUserList.end(), webUserList[sel]));
		ctrlWebUsers.DeleteItem(sel);
	}

	return 0;
}

void WebServerPage::setLastError(const string& aError) noexcept {
	lastError = Text::toT(aError);
}

LRESULT WebServerPage::onSelChange(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
	if (ctrlWebUsers.GetSelectedCount() == 1) {
		ctrlChange.EnableWindow(TRUE);
		ctrlRemove.EnableWindow(TRUE);
	} else {
		ctrlChange.EnableWindow(FALSE);
		ctrlRemove.EnableWindow(FALSE);
	}
	return 0;
}
LRESULT WebServerPage::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*)pnmh;
	switch (kd->wVKey) {
	case VK_INSERT:
		PostMessage(WM_COMMAND, IDC_WEBSERVER_ADD_USER, 0);
		break;
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_WEBSERVER_REMOVE_USER, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

LRESULT WebServerPage::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if (item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_WEBSERVER_CHANGE, 0);
	}
	else if (item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_WEBSERVER_ADD_USER, 0);
	}

	return 0;
}

void WebServerPage::updateState(ServerState aNewState) noexcept {
	tstring statusText;
	if (aNewState == STATE_STARTED) {
		ctrlStart.SetWindowText(CTSTRING(STOP));

		if (!lastError.empty()) {
			statusText += lastError + _T("\n");
		}

		statusText += TSTRING(WEB_SERVER_RUNNING);
	} else if(aNewState == STATE_STOPPING) {
		statusText += TSTRING(STOPPING);
		//ctrlStatus.SetWindowText(_T("Stopping..."));
	} else if (aNewState == STATE_STOPPED) {
		ctrlStart.SetWindowText(CTSTRING(START));

		if (lastError.empty()) {
			statusText += TSTRING(WEB_SERVER_STOPPED);
		} else {
			statusText += TSTRING_F(WEB_SERVER_START_FAILED, lastError.c_str());
		}
	}

	ctrlStatus.SetWindowText(statusText.c_str());


	ctrlStart.EnableWindow(aNewState == STATE_STOPPING ? FALSE : TRUE);
	currentState = aNewState;
}

void WebServerPage::write() {
	auto plainserverPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlPort)));
	auto tlsServerPort = Util::toInt(Text::fromT(WinUtil::getEditText(ctrlTlsPort)));

	auto needServerRestart = webMgr->getPlainServerConfig().getPort() != plainserverPort || webMgr->getTlsServerConfig().getPort() != tlsServerPort;

	applySettings();
	webMgr->save();

	if (needServerRestart) {
		if (webMgr->isRunning()) {
			webMgr->getInstance()->stop();
		}

		webMgr->getInstance()->start([](const string& aError) {
			LogManager::getInstance()->message(aError, LogMessage::SEV_ERROR);
		});
	}
}

void WebServerPage::addListItem(const string& aUserName, const string& aPassword) {
	int p = ctrlWebUsers.insert(ctrlWebUsers.GetItemCount(), Text::toT(aUserName));
	ctrlWebUsers.SetItemText(p, 1, Text::toT(aPassword).c_str());
}

WebServerPage::~WebServerPage() {
	webMgr->removeListener(this);
	webUserList.clear();
	ctrlWebUsers.Detach();
};
