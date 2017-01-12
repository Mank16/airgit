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

#ifndef DCPLUSPLUS_DCPP_SEARCHINSTANCE_H
#define DCPLUSPLUS_DCPP_SEARCHINSTANCE_H

#include "stdinc.h"

#include "ClientManagerListener.h"
#include "SearchInstanceListener.h"
#include "SearchManagerListener.h"

#include "GroupedSearchResult.h"
#include "SearchQuery.h"
#include "Speaker.h"


namespace dcpp {
	struct SearchQueueInfo;
	class SearchInstance : public Speaker<SearchInstanceListener>, private SearchManagerListener, private ClientManagerListener {
	public:
		SearchInstance();
		~SearchInstance();

		SearchQueueInfo hubSearch(StringList& aHubUrls, const SearchPtr& aSearch) noexcept;
		bool userSearch(const HintedUser& aUser, const SearchPtr& aSearch, string& error_) noexcept;

		void reset(const SearchPtr& aSearch) noexcept;

		const string& getCurrentSearchToken() const noexcept {
			return currentSearchToken;
		}

		GroupedSearchResultList getResultList() const noexcept;

		// The most relevant result is sorted first
		GroupedSearchResult::Set getResultSet() const noexcept;
		GroupedSearchResult::Ptr getResult(GroupedResultToken aToken) const noexcept;

		uint64_t getTimeFromLastSearch() const noexcept;
		int getQueueCount() const noexcept;
		int getResultCount() const noexcept;
		uint64_t getQueueTime() const noexcept;
	private:
		void on(SearchManagerListener::SR, const SearchResultPtr& aResult) noexcept override;

		GroupedSearchResult::Map results;
		shared_ptr<SearchQuery> curSearch;
		StringSet queuedHubUrls;

		std::string currentSearchToken;
		mutable SharedMutex cs;

		void on(ClientManagerListener::OutgoingSearch, const string& aHubUrl, const SearchPtr& aSearch) noexcept;
		void on(ClientManagerListener::ClientDisconnected, const string& aHubUrl) noexcept;

		void removeQueuedUrl(const string& aHubUrl) noexcept;
		uint64_t lastSearchTime = 0;
		int searchesSent = 0;
	};
}

#endif