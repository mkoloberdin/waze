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
   Mood_Bronze,
   Mood_Silver,
   Mood_Gold,
   Mood_Busy,
   Mood_Busy_Female,
   Mood_In_a_Hurry,
   Mood_In_a_Hurry_Female,
   Mood_Baby,
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
#define  MOOD_Name_Happy_Female      ("happy-female")
#define  MOOD_Name_Sad_Female        ("sad-female")
#define  MOOD_Name_Mad_Female        ("mad-female")
#define  MOOD_Name_Bored_Female      ("bored-female")
#define  MOOD_Name_Speedy_Female     ("speedy-female")
#define  MOOD_Name_Starving_Female   ("starving-female")
#define  MOOD_Name_Sleepy_Female     ("sleepy-female")
#define  MOOD_Name_Cool_Female       ("cool-female")
#define  MOOD_Name_InLove_Female     ("inlove-female")
#define  MOOD_Name_LOL_Female        ("LOL-female")
#define  MOOD_Name_Peaceful_Female   ("peaceful-female")
#define  MOOD_Name_Singing_Female    ("singing-female")
#define  MOOD_Name_Wondering_Female  ("wondering-female")
#define  MOOD_Name_Bronze            ("wazer_bronze")
#define  MOOD_Name_Silver            ("wazer_silver")
#define  MOOD_Name_Gold              ("wazer_gold")
#define  MOOD_Name_Baby              ("wazer_baby")
#define  MOOD_Name_Busy              ("busy")
#define  MOOD_Name_In_A_Hurry        ("in_a_hurry")
#define  MOOD_Name_Busy_Female       ("busy-female")
#define  MOOD_Name_In_A_Hurry_Female ("in_a_hurry-female")

void roadmap_mood_init(void);
void roadmap_mood (void);
void roadmap_mood_dialog (RoadMapCallback callback);
const char *roadmap_mood_get();
const char *roadmap_mood_get_name();
void roadmap_mood_set(const char *value);
const char *roadmap_mood_get_top_name();
int roadmap_mood_state(void);
int roadmap_mood_actual_state(void);
void roadmap_mood_set_exclusive_moods(int mood);
void roadmap_mood_set_override_mood(int mood);
int roadmap_mood_get_exclusive_moods(void);

const char* roadmap_mood_to_string(MoodType mood);
MoodType roadmap_mood_from_string( const char* szMood);
