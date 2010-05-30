/* roadmap_iphonescoreboard.h - iPhone scoreboard view
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ROADMAP_IPHONESCOREBOARD__H
#define __ROADMAP_IPHONESCOREBOARD__H


@interface ScoreboardMyView : UIView {
   UIButton    *showMeButton;
   UILabel     *youLabel;
   UILabel     *pointsLabel;
   UIImageView *trendImageV;
   UIImageView *specialImageV;
   UIActivityIndicatorView *activityIndicator;
   UILabel     *deltaLabel;
   UILabel     *leftLabel;
   UILabel     *rightLabel;
}

- (void) clearContent;

@end

@interface ScoreboardRowView : UIView {
   UILabel     *nameLabel;
   UILabel     *attribLabel;
   UILabel     *pointsLabel;
   UIImageView *trendImageV;
   UIImageView *specialImageV;
   UIActivityIndicatorView *activityIndicator;
   UILabel     *deltaLabel;
   BOOL        isHighlighted;
}

- (void) clearContent;

@end

@interface ScoreboardDialog : UITableViewController {
   int         lastRank;
}


- (void) show;
- (void) refreshMy;
- (void) refreshScores;
- (void) scrollToRank: (int)rank;

@end


#endif // __ROADMAP_IPHONESCOREBOARD__H