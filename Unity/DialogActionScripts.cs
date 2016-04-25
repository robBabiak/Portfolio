/**
 *  StarHaven Project - CMPT 330
 *  Grant MacEwan University 2015
 *  Members: Robert Babiak, Justyn Huculak, Winston Kouch, Ian Nalbach
 *
 *
 * This code is used for dialog action processing. ie. setting story bits,
 * changing cash, reputation and making states that allow for NPCs to appear.
 * It can also set the game time, clear bits, grab names and or grab NPC genders.
 * 
 * 
 * Public Objects		Description
 * 
 * dialogID				Associated dialog ID of story nodes
 * scene				Categorized by our scenes ie. Lord Balanor
 * 
 * Private Objects		Description
 * 
 * story				Story data contents so we can update things such as story bits, reputation and cash
 * globalData			Global data stores our game world time so we can update time of game world
 *
 */

using UnityEngine;
using System.Collections;
using System;
using System.Reflection;

public partial class DialogActionScripts : MonoBehaviour
{
    private StoryData story;
    private GlobalData globalData;
    void Start()
    {
        story = (StoryData) gameObject.GetComponent("StoryData");
        globalData = gameObject.GetComponent<GlobalData>();
    }
    void Update()
    {

    }
	// Call method to use the functions we made such as IsBitOn in relation to the dialog node
    public bool CallFragment(string func, System.Int64 id)
    {
        string funcName = func + "_" + id;
        MethodInfo checkFunc = this.GetType().GetMethod(funcName, BindingFlags.NonPublic | BindingFlags.Instance);
        if (checkFunc != null)
        {
            // Debug.Log("Calling :" + funcName);
            return (bool) checkFunc.Invoke(this, null);
        }
        else
        {
            Debug.Log("ERROR Missing Check Function " + funcName);
        }
        return true;

    }
	// Retrieve Pawn by search using pawn's name
    private Pawn GetPawnByName(string pawnName)
    {
        Debug.Log("FindPawnByName " + pawnName);
        GameObject obj = GameObject.Find(pawnName);
        if (obj)
        {
            Debug.Log("Found object " + obj + "  " + obj.GetComponent<Pawn>());
            return obj.GetComponent<Pawn>();
        }
        Debug.Log("Failed to find pawn");
        return null;
    }
	// Retrieve NPC Pawn Object for associated node
    private Pawn GetPawn()
    {
        DialogManager mgr = (DialogManager)gameObject.GetComponent<DialogManager>();
        if (mgr) return mgr.GetPawnForCurrentNode();
        return null;
    }
	// Set the time of the game world
    private void SetGameTime(float hour)
    {
        globalData.Hour = hour;
    }
    // -----------------------------------------------------------------------------------
    // Helper functions
    // The bit operations
    private bool IsBitOn(BIT bit)
    {
        return story.on(bit);
    }
    private bool IsBitOff(BIT bit)
    {
        return story.off(bit);
    }
    private void ClearBit(BIT bit)
    {
        story.assign(bit, false);
    }
    private void SetBit(BIT bit)
    {
        story.assign(bit, true);
    }
    // the int operations
    private int GetInt(INTS idx)
    {
        return story.getCount(idx);
    }
    private void SetInt(INTS idx, int val)
    {
        story.setCount(idx, val);
    }
    private void IncInt(INTS idx)
    {
        story.incCount(idx);
    }
    private void DecInt(INTS idx)
    {
        story.decCount(idx);
    }
    private void AdjustInt(INTS idx, int value)
    {
        story.setCount(idx, story.getCount(idx) + value);
    }
    // the float operations
    private float GetFloat(FLOATS idx)
    {
        return story.getCount(idx);
    }
    private void SetFloat(FLOATS idx, float val)
    {
        story.setCount(idx, val);
    }
    private void RandomizeFloat(FLOATS idx)
    {
        story.RandomizeFloat(idx);
    }
    // Persons of interest
    private string NameOf(string nameOfPerson)
    {
        return story.getPersonsOfIntrest(nameOfPerson);
    }
    private bool IsMale(string nameOfPerson)
    {
        return story.getPersonsOfIntrestGender(nameOfPerson);
    }

    // -----------------------------------------------------------------------------------
    // Functions for common operations
	// Adjust the reputation of the player based on his decisions
    private void AdjustReputation(int value)
    {
        AdjustInt(INTS.reputation, value);
    }

}
