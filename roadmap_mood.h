/* roadmap_mood.h
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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

//////////////////////////////////////////////////////////////////////////////////////////////////
// MoodsTypes
typedef enum MoodType
{
   Mood_Happy = 1,
   Mood_Sad,
   Mood_Mad,
   Mood_Bored,
   Mood_Speedy,
   Mood_Starving,
   Mood_Sleepy,
   Mood_Cool,
   Mood_InLove,
   Mood_LOL,
   Mood_Peaceful,
   Mood_Singing,
   Mood_Wondering,
   Mood_Happy_Female,
   Mood_Sad_Female,
   Mood_Mad_Female,
   Mood_Bored_Female,
   Mood_Speedy_Female,
   Mood_Starving_Female,
   Mood_Sleepy_Female,
   Mood_Cool_Female,
   Mood_InLove_Female,
   Mood_LOL_Female,
   Mood_Peaceful_Female,
   Mood_Singing_Female,
   Mood_Wondering_Female,

   Mood__count,
   Mood__invalid
   
}  MoodType;


#define  MOOD_Name_Happy     ("happy")
#define  MOOD_Name_Sad       ("sad")
#define  MOOD_Name_Mad       ("mad")
#define  MOOD_Name_Bored     ("bored")
#define  MOOD_Name_Speedy    ("speedy")
#define  MOOD_Name_Starving  ("starving")
#define  MOOD_Name_Sleepy    ("sleepy")
#define  MOOD_Name_Cool      ("cool")
#define  MOOD_Name_InLove    ("inlove")
#define  MOOD_Name_LOL       ("LOL")
#define  MOOD_Name_Peaceful  ("peaceful")
#define  MOOD_Name_Singing   ("singing")
#define  MOOD_Name_Wondering ("wondering")
#define  MOOD_Name_Happy_Female     ("happy-female")
#define  MOOD_Name_Sad_Female       ("sad-female")
#define  MOOD_Name_Mad_Female       ("mad-female")
#define  MOOD_Name_Bored_Female     ("bored-female")
#define  MOOD_Name_Speedy_Female    ("speedy-female")
#define  MOOD_Name_Starving_Female  ("starving-female")
#define  MOOD_Name_Sleepy_Female    ("sleepy-female")
#define  MOOD_Name_Cool_Female      ("cool-female")
#define  MOOD_Name_InLove_Female    ("inlove-female")
#define  MOOD_Name_LOL_Female       ("LOL-female")
#define  MOOD_Name_Peaceful_Female  ("peaceful-female")
#define  MOOD_Name_Singing_Female   ("singing-female")
#define  MOOD_Name_Wondering_Female ("wondering-female")

#define  MOOD_Name_Happy_Small     ("mood_happy_small")
#define  MOOD_Name_Sad_Small       ("mood_sad_small")
#define  MOOD_Name_Mad_Small       ("mood_mad_small")
#define  MOOD_Name_Bored_Small     ("mood_bored_small")
#define  MOOD_Name_Speedy_Small    ("mood_speedy_small")
#define  MOOD_Name_Starving_Small  ("mood_starving_small")
#define  MOOD_Name_Sleepy_Small    ("mood_sleepy_small")
#define  MOOD_Name_Cool_Small      ("mood_cool_small")
#define  MOOD_Name_InLove_Small    ("mood_inlove_small")
#define  MOOD_Name_LOL_Small       ("mood_LOL_small")
#define  MOOD_Name_Peaceful_Small  ("mood_peaceful_small")
#define  MOOD_Name_Singing_Small   ("mood_singing_small")
#define  MOOD_Name_Wondering_Small ("mood_wondering_small")
#define  MOOD_Name_Happy_Small_Female     ("mood_happy_small_female")
#define  MOOD_Name_Sad_Small_Female       ("mood_sad_small_female")
#define  MOOD_Name_Mad_Small_Female       ("mood_mad_small_female")
#define  MOOD_Name_Bored_Small_Female     ("mood_bored_small_female")
#define  MOOD_Name_Speedy_Small_Female    ("mood_speedy_small_female")
#define  MOOD_Name_Starving_Small_Female  ("mood_starving_small_female")
#define  MOOD_Name_Sleepy_Small_Female    ("mood_sleepy_small_female")
#define  MOOD_Name_Cool_Small_Female      ("mood_cool_small_female")
#define  MOOD_Name_InLove_Small_Female    ("mood_inlove_small_female")
#define  MOOD_Name_LOL_Small_Female       ("mood_LOL_small_female")
#define  MOOD_Name_Peaceful_Small_Female  ("mood_peaceful_small_female")
#define  MOOD_Name_Singing_Small_Female   ("mood_singing_small_female")
#define  MOOD_Name_Wondering_Small_Female ("mood_wondering_small_female")

void roadmap_mood (void);
void roadmap_mood_dialog (RoadMapCallback callback);
const char *roadmap_mood_get();
const char *roadmap_mood_get_name();
void roadmap_mood_set(const char *value);
const char *roadmap_mood_get_top_name();
int roadmap_mood_state(void);

const char* roadmap_mood_to_string(MoodType mood);
MoodType roadmap_mood_from_string( const char* szMood);
const char* roadmap_mood_to_string_small(MoodType mood);
