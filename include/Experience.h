//
// Copyright (c) 2003-2009, by Yet Another POD-Bot Development Team.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Id$
//

#ifndef EXPERIENCE_INCLUDED
#define EXPERIENCE_INCLUDED

//
// Variable: MAX_EXPERIENCE_VALUE
// Maximum damage value for experience.
//
const uint16_t MAX_EXPERIENCE_VALUE = 2048;

//
// Variable: MAX_KHIST_VALUE
// Maximum kill history for experience.
//
const uint16_t MAX_KHIST_VALUE = 16;

//
// Class: BotExperience
// Implements bot experience class.
//
class BotExperience
{
    //
    // Group: Private Members.
    //
private:

    //
    // Struct: ExpData
    // Represents experience data for each waypoint.
    //
    struct ExpData
    {
        //
        // Variable: damage
        // Damage for waypoint.
        //
        uint16_t damage[TEAM_COUNT];

        //
        // Variable: danger
        // Danger index of that waypoint.
        //
        int16 danger[TEAM_COUNT];

        //
        // Variable: value
        // Goal value for that waypoint.
        //
        int16 value[TEAM_COUNT];
    };

    //
    // Variable: m_data
    // Holds all in-game experience info for waypoints.
    //
    ExpData* m_data;

    //
    // Variable: m _history
    // Kill history for experience.
    //
    uint16_t m_history;

public:
    BotExperience(void) { }

    inline ~BotExperience(void)
    {
        if (m_data != null)
            delete[] m_data;

        m_data = null;
    }

    //
    // Group: Public accessible methods.
    //
public:

    //
    // Function: UpdateGlobalKnowledge
    //
    // Description here.
    //
    // Parameters:
    //    - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    void UpdateGlobalKnowledge(void);

    //
    // Function: CollectDamage
    //
    // Description here.
    //
    // Parameters:
    //   victim - 
    //   attacker - 
    //   health - 
    //   damage - 
    //   vicVal - 
    //   attVal - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    void CollectDamage(const Client& victim, const Client& attacker, int health, int damage, float& vicVal, float& attVal);

    //
    // Function: CollectValidDamage
    //
    // Description here.
    //
    // Parameters:
    //   index - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    void CollectValidDamage(int index, int team);

    //
    // Function: Load
    //
    // Description here.
    //
    // Parameters:
    //    - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    void Load(void);

    //
    // Function: Unload
    //
    // Description here.
    //
    // Parameters:
    //    - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    void Unload(void);

    //
    // Function: DrawText
    //
    // Description here.
    //
    // Parameters:
    //   storage - 
    //   length - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    void DrawText(int index, char storage[4096], int& length);

    //
    // Function: DrawLines
    //
    // Description here.
    //
    // Parameters:
    //    - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    void DrawLines(int nearest, Path* path);

    //
    // Function: CollectGoal
    //
    // Description here.
    //
    // Parameters:
    //   health - 
    //   damage - 
    //   goal - 
    //   prevGoal - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    void CollectGoal(int health, int damage, int goal, int prevGoal, int team);

    //
    // Function: ColletValue
    //
    // Description here.
    //
    // Parameters:
    //   start - 
    //   goal - 
    //   health - 
    //   goal - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    void CollectValue(int start, int goal, int health, float goalValue);

    //
    // Function: GetDamage
    //
    // Description here.
    //
    // Parameters:
    //   start - 
    //   goal - 
    //   team - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    inline uint16_t GetDamage(int start, int goal, int team) const
    {
        return (m_data + (start * g_numWaypoints) + goal)->damage[team]; // just return data
    }

    //
    // Function: GetValue
    //
    // Description here.
    //
    // Parameters:
    //   start - 
    //   goal - 
    //   team - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    inline int16 GetValue(int start, int goal, int team) const
    {
        return (m_data + (start * g_numWaypoints) + goal)->value[team]; // just return data
    }

    //
    // Function: GetDangerIndex
    //
    // Description here.
    //
    // Parameters:
    //   start - 
    //   goal - 
    //   team - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    inline int16 GetDangerIndex(int start, int goal, int team) const
    {
        return (m_data + (start * g_numWaypoints) + goal)->danger[team]; // just return data
    }

    inline int GetKillHistory(void)
    {
        return m_history;
    }

    inline float GetAStarValue(int point, int team, bool dist)
    {
        return static_cast <float> ((m_data + (point * g_numWaypoints) + point)->damage[team] + (dist ? m_history : 0)); // just return data
    }
    //
    // Group: Private Functions.
    //
private:
    //
    // Function: SetDamage
    //
    // Description here.
    //
    // Parameters:
    //   start - 
    //   goal - 
    //   newValue - 
    //   team - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    void SetDamage(int start, int goal, int newValue, int team);

    //
    // Function: SetValue
    //
    // Description here.
    //
    // Parameters:
    //   start - 
    //   goal - 
    //   newValue - 
    //   team - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    void SetValue(int start, int goal, int newValue, int team);

    //
    // Function: SetDangerIndex
    //
    // Description here.
    //
    // Parameters:
    //   start - 
    //   goal - 
    //   newIndex - 
    //   team - 
    //
    // Returns:
    //   
    //
    // Remarks:
    //   
    //
    void SetDangerIndex(int start, int goal, int newIndex, int team);
};

extern BotExperience g_exp;


#endif // EXPERIENCE_INCLUDED