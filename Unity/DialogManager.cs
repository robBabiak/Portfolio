/**
 *  StarHaven Project - CMPT 330
 *  Grant MacEwan University 2015
 *  Members: Robert Babiak, Justyn Huculak, Winston Kouch, Ian Nalbach
 *
 *
 * This code is used to manage all the game's dialogs.
 * It can handle dialog quest trees, store associated information on NPCs, dialog
 * check requirements, action functions, and perform SQL operations on our sqlite database.
 * Each quest node allows several pathways a player can take. This system acknowledges them
 * as response nodes.
 *
 *
 * Public Objects		Description
 *
 * dialogID				Associated dialog ID of story nodes
 * scene				Categorized by our scenes ie. Lord Balanor
 *
 * Private Objects		Description
 *
 * db					SQLite Databasefile of quest flow (dialogs)
 *
 */

using UnityEngine;
using System;
using System.Collections;
using SQLiter;
using System.Collections.Generic;
using UnityEngine.UI;
using System.Text.RegularExpressions;
using System.Globalization;

using System.CodeDom;
using System.Reflection;
using System.CodeDom.Compiler;
using Mono.CSharp;

// using System.Diagnostics;

[Serializable]
public class DialogNode

{
    private SQLite db;

    public readonly bool            isValid = false;        // Is the node loaded

    public readonly System.Int64    id;                     // the dialog node ID
    public readonly bool            entry;                  // Is this a entry node
    public readonly bool            barkString;             // is this a bark string
    public readonly string          text;                   // The text of the response
    public readonly string          checkFunction;          // The function to test if this node should be displayed.
    public readonly string          actionFunction;         // action that occurs on the activation of this node
    public readonly int             speaker;                // who is the speaker of this response
    public readonly float           time;                   // Voice over duration
    public readonly string          voiceFile;              // Voice over File
    public readonly int             mood;                   // What is the tone of the response

    public readonly List<System.Int64> responseIds;         // the list of response nodes.


    // -----------------------------------------------------------------------------------
    // Initialize the node and read this nodes data.
    public DialogNode(System.Int64 _id, SQLite _db)
    {
        id = _id;
        db = _db;
        responseIds = new List<System.Int64>();
        isValid = false;
        // A quirk of C# is that read only public property can only be initialized at deceleration
        // or in the class constructor. So I am going strait to the DB here. Normally this would be
        // bad as constructors don't generality handle errors well... But since C# can raise errors from
        // a constructor, we should catch them.
        List<Dictionary<string, object>> records;
        records = db.SQL("SELECT * from dialog where id = " + id.ToString());
        if (records.Count == 1)
        {
            entry           = ((System.Int64)records[0]["entry"] != 0);
            barkString      = ((System.Int64)records[0]["barkString"] != 0);
            text            = (string)records[0]["text"];
            checkFunction   = (records[0]["checkFunction"] != null ? (string)records[0]["checkFunction"] : null);
            actionFunction  = (records[0]["actionFunction"] != null ? (string)records[0]["actionFunction"] : null);
            speaker         = (records[0]["speaker"] != null ? (int)records[0]["speaker"] : 0);

            // UnityEngine.Debug.Log("Time " + records[0]["time"]);

            // time            = (records[0]["time"] != null ? (float)records[0]["time"] : 0.0f);

            voiceFile       = (string)records[0]["voiceFile"];
            mood            = (records[0]["mood"] != null ? (int)records[0]["mood"] : 0);

            // find the responseText.
            records = db.SQL("SELECT * from dialogTree where id = " + id.ToString());
            foreach (Dictionary<string, object> row in records)
            {
                responseIds.Add((System.Int32)row["child"]);
            }
            isValid = true;
        }

    }

    // -----------------------------------------------------------------------------------
    // This will iterate the responseText, and construct dialog nodes for each, returning them.
    // as a list. This node doesn't hold a copy of the constructed responseText, so any repeate
    // queries will re fetch. This is because the dialog nodes are part of a directed graph
    // that can have loop back links. and these typically make garbage collection
    // chock for a bit while they sort out the circular references.
    public List<DialogNode> GetResponseNodes()
    {
        List<DialogNode> res = new List<DialogNode>();
        responseIds.ForEach(delegate(System.Int64 childID)
        {
            res.Add(new DialogNode(childID, db));
        });

        return res;
    }

    // -----------------------------------------------------------------------------------
    // When we ask for hte string, show the contents of the node.
    public override string ToString()
    {
        string children = "";
        responseIds.ForEach(delegate(System.Int64 childID)
        {
            children += childID.ToString() + ", ";
        });

        return string.Format( "Text:{1}\nNode:{0} Mood:{7}\nScript:({2},{3})\nVO:[{4}:{5}]\nChildIds:[{6}]",
            id,             // Node: {0}
            text,           // Text: {1}
            checkFunction,  // Script:( {2}
            actionFunction, // , {3})
            voiceFile,      // VO:[ {4}
            time,           // : {5}]
            children,       // ChildIDs:[{6}]
            mood            // Mood:{7}
        );

    }
}
// -----------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------
public class DialogManager : MonoBehaviour
{
    public bool log = false;
    private bool dialogActive = false;
    public GameObject dialogPanel;      // The dialog panel that is displayed and hidden when dialog begins and ends.
    public Text textField;              // The dialog Text
    public List<Button> responseBtns;   // the buttons that are the inputs
    public List<Text> responseText;     // the text int he buttons that are the mesage
    public Button previous;             // Previous screen If displaying more than one
    public Button next;                 // Next screen if more than 4 responseText.
    public Image image;                 // The Image from a dialog node.

    public Color neutralResponse = Color.white;
    public Color hostileResponse = Color.red;
    public Color friendlyResponse = Color.blue;

    private DialogNode currentNode;
    private List<DialogNode> responseNodes;
    public int currentRootNodeID;
    public int currentNodeID;
    private int currentPage = 0;

    public DialogScene scene;
    private SQLite db;
    private StoryData story;
    private DialogActionScripts scriptFragments; // This is the action scripts

    private Player player;
    private Camera mainCam;
    private GameObject miniMap;
	// Use this for initialization
	void Start ()
    {
        db = (SQLite) gameObject.GetComponent("SQLite");
        story = (StoryData) gameObject.GetComponent("StoryData");
        player = GameObject.FindWithTag("Player").GetComponent<Player>();
        scriptFragments = (DialogActionScripts) gameObject.GetComponent<DialogActionScripts>();
        mainCam = GameObject.FindWithTag("MainCamera").GetComponent<Camera>();
        miniMap = GameObject.Find("MiniMap");

    }

	// Update is called once per frame
	void Update ()
    {
        // The buttons are 1-5, while the events are zero based 0-4
        if (dialogActive)
        {
            // If the previous button is active, then it is the previous page
            if ( Input.GetButtonDown("Dialog1") )
            {
                if (previous.gameObject.activeSelf)
                {
                    PrevPageClicked();
                }
                else // else we clicked the first response
                {
                    ResponseClicked(0);
                }
            }
            if ( Input.GetButtonDown("Dialog2") )
            {
                ResponseClicked(1);
            }
            if ( Input.GetButtonDown("Dialog3") )
            {
                ResponseClicked(2);
            }
            if ( Input.GetButtonDown("Dialog4") )
            {
                ResponseClicked(3);
            }
            if ( Input.GetButtonDown("Dialog5") )
            {
                if (next.gameObject.activeSelf) // if the previous is active, then we go to the previous page
                {
                    NextPageClicked();
                }
                else // else select the fourth response
                {
                    ResponseClicked(4);
                }
            }
            if ( Input.GetButtonDown("Cancel") )
            {
                EndDialog();
            }
        }
	}

    // -----------------------------------------------------------------------------------
    // Begin a Dialog with the player
    // This is called from a external source to start a dialog happening
    public bool BeginDialog(System.Int64 entryPoint)
    {

        scene = null;
        if (dialogActive) return false; // We don't want to start a new dialog while one is active.
        if (log) UnityEngine.Debug.Log("Begin Dialog " + entryPoint + " fake scene");
        story.GenerateRandom();
        currentRootNodeID = (int)entryPoint;
        DialogNode node = new DialogNode(entryPoint, db);
        if (log) UnityEngine.Debug.Log("Node Valid " + node.isValid);
        if (node.isValid)
        {
            if (miniMap) miniMap.SetActive(false);
            player.BeginDialog();
            dialogActive = true;
            dialogPanel.SetActive(true);
            DisplayDialog(node);
        }
        return true;
    }
    // -----------------------------------------------------------------------------------
    // Begin a Dialog with the player
    // This is called from a external source to start a dialog happening
    public bool BeginDialog(System.Int64 entryPoint, DialogScene dialogScene)
    {
        if (dialogActive) return false; // We don't want to start a new dialog while one is active.
        if (log) UnityEngine.Debug.Log("Begin Dialog " + entryPoint + " scene " + dialogScene);
        scene = dialogScene;
        story.GenerateRandom();
        currentRootNodeID = (int)entryPoint;
        DialogNode node = new DialogNode(entryPoint, db);
        if (log) UnityEngine.Debug.Log("Node Valid " + node.isValid);
        if (node.isValid)
        {
            if (miniMap) miniMap.SetActive(false);
            player.BeginDialog();
            dialogActive = true;
            dialogPanel.SetActive(true);
            DisplayDialog(node);
        }
        return true;

    }


    // -----------------------------------------------------------------------------------
    // End a dialog has occured.
    void EndDialog()
    {
        if (miniMap) miniMap.SetActive(true);
        player.EndDialog();
        dialogPanel.SetActive(false);
        dialogActive = false;

    }

    // -----------------------------------------------------------------------------------
    // Return the bark string randomly selected from the list of returned ones.
    public DialogNode GetBarkString(System.Int64 entryPoint)
    {
        DialogNode node = new DialogNode(entryPoint, db);
        if (node.isValid)
        {
            List<DialogNode> nodes = FilterBarkNodes(node.GetResponseNodes());
            if (nodes.Count != 0)
            {
                // string msg = nodes[UnityEngine.Random.Range(0, nodes.Count)].text;

                // return ReplaceMacros(msg, false);
                return nodes[UnityEngine.Random.Range(0, nodes.Count)];
            }

        }
        return null;

    }
    // -----------------------------------------------------------------------------------
    // This will filter dialog nodes and remove barkstring nodes, and any node that fails to
    // pass the check funciton.
    List<DialogNode> FilterDialogNodes(List<DialogNode> nodes)
    {
        return FilterNodesForType(nodes, false);
    }
    List<DialogNode> FilterBarkNodes(List<DialogNode> nodes)
    {
        return FilterNodesForType(nodes, true);
    }
    List<DialogNode> FilterNodesForType(List<DialogNode> nodes, bool barkStringOnly )
    {
        List<DialogNode>res = new List<DialogNode>();
        foreach( DialogNode n in nodes)
        {
            // if bark string then it is not part of a dialog. These nodes are handled differently.
            if (n.barkString != barkStringOnly) continue;
            // Call the check function, if it returns false,
            // then the node should not be added to list.
            if (scriptFragments.CallFragment("check", n.id) == false)
            {
                continue;
            }
            // check activation script
            res.Add(n);
        }
        return res;
    }

    // -----------------------------------------------------------------------------------
    // Display the dialog node on the UI
    void DisplayDialog(DialogNode node)
    {
        currentNode = node;
        currentNodeID = (int)node.id;
        currentPage = 0;
        image.sprite = null;
        image.color = new Color(1.0f, 1.0f, 1.0f, 0.0f);
        responseNodes = FilterDialogNodes(currentNode.GetResponseNodes());
        textField.text =  ReplaceMacros(currentNode.text, false);
        if (scene != null)
        {
            if (log) Debug.Log("Set Camera to " +currentNode.speaker);
            scene.SetCamera(mainCam, currentNode.speaker);
        }
        if (responseNodes.Count == 0)
        {
            EndDialog();
        }
        if (image.sprite != null)
        {
            image.color = new Color(1.0f, 1.0f, 1.0f, 1.0f);
        }

        DisplayResponses(currentPage);
        if (scriptFragments.CallFragment("action", currentNode.id) == false)
        {
            EndDialog();
            return;
        }
    }

    // -----------------------------------------------------------------------------------
    // Display the UI response buttones
    void DisplayResponses(int page)
    {
        previous.gameObject.SetActive(false);

        for (int i = 0; i< 4; ++i)
        {
            responseBtns[i].gameObject.SetActive(false);
        }
        int first = 0;
        if (page > 0)
        {
            previous.gameObject.SetActive(true);
            first = 1;
        }

        for (int i = 0; i< responseNodes.Count; ++i)
        {
            if (i < page) continue;

            responseBtns[first].gameObject.SetActive(true);

            responseText[first].text = "<color=white><b>" + (first+1).ToString()+".</b></color> " + ReplaceMacros(responseNodes[i].text, true);

            switch(responseNodes[i].mood)
            {
                case 1: responseText[first].color = friendlyResponse; break;
                case 2: responseText[first].color = hostileResponse; break;
                default: responseText[first].color = neutralResponse; break;
            }
            if (++first > 3) break;
            // TODO:Set color, based on the Mood
        }
        next.gameObject.SetActive((responseNodes.Count - page) > 3 );
    }

    // -----------------------------------------------------------------------------------
    // Find macros and expand them.
    static Regex rgxMacro = new Regex(@"\{\{(\S*)\:([0-9a-zA-Z _\-,\(\)]*)\}\}");
    public string ReplaceMacros (string text, bool responseMode)
    {
        int         istart = 0;
        string      newText = text;
        Match match = rgxMacro.Match(newText);
        // if (log) Debug.Log("Macro replace" + match.Success + "  " + text);
        while (match.Success)
        {
            // We found a mcaro string, now replace it.
            // if (log) Debug.Log("Macro " +match.Groups[1].Value+ "   " + match.Groups[2].Value);
            newText = rgxMacro.Replace(newText,ValueForMacro(match.Groups[1].Value, match.Groups[2].Value, responseMode) ,1,istart);
            match = match.NextMatch();
        }
        return newText;
    }

    private string ValueForMacro(string key, string value, bool responseMode)
    {
        string argValue = value;
        string argument = "";
        int spos = value.IndexOf("(");
        int epos = value.IndexOf(")");
        if (spos != -1 && epos != -1 && argValue.Substring(spos-1,1) != "\\" && argValue.Substring(epos-1,1) != "\\")
        {
            // We have a parameter
            argument = argValue.Substring(spos+1,epos-spos-1);
            argValue = argValue.Substring(0,spos);

        }
        switch (key)
        {
            case "IMAGE": return ValueForImage(argValue, responseMode,argument);
            case "PERSON": return ValueForPerson(argValue, responseMode, argument);
            case "PERSON-GENDER": return ValueForPersonGender(argValue, responseMode, argument);
            case "ANIM": return ValueForAnim(argValue, responseMode, argument);
            default: return "<<BAD_MACRO "+ key +" - " +value + ">>";
        }

    }
    private string ValueForImage(string value, bool responseMode, string argument)
    {
        // Argument is not used.
        if (responseMode == false)
        {
            // Load the sprite into the image.
            image.sprite = Resources.Load <Sprite>("DialogImages/" + value);
        }
        // eat the string
        return "";
    }
    private string ValueForAnim(string value, bool responseMode, string argument)
    {
        PawnEmote anim = (PawnEmote)Convert.ToInt32(value);
        Debug.Log("ANIM ==== " + value + "  " + responseMode + "  " + argument + "  " + anim);
        if (scene != null && responseMode == false)
        {
            scene.PlayAnim(currentNode.speaker, anim);
        }
        return "";
    }
    public Pawn GetPawnForCurrentNode()
    {
        if (scene != null)
        {
            return scene.GetPawn(currentNode.speaker);
        }
        return null;
    }
    private string ValueForPerson(string value, bool responseMode, string argument)
    {
        string person = story.getPersonsOfIntrest(value) ;
        if (person != null)
        {
            if (argument.Length > 0)
            {
                TextInfo textInfo = new CultureInfo("en-US", false).TextInfo;
                if (argument.ToLower() == "title")
                {
                    person = textInfo.ToTitleCase(person);
                }
                if (argument.ToLower() == "lower")
                {
                    person = textInfo.ToLower(person);
                }
                if (argument.ToLower() == "upper")
                {
                    person = textInfo.ToUpper(person);
                }
            }
        }
        // Debug.Log("Value for Person " + value + " = " + person);
        if (person == null || person.Length == 0) person = "<<Missing:" + value +">>";
        return person;
    }

    private string ValueForPersonGender(string value, bool responseMode, string argument)
    {
        if (argument.Length > 0 && argument.IndexOf(",") != -1)
        {
            bool isMale = story.getPersonsOfIntrestGender(value) ;
            return argument.Split(',')[isMale ? 0: 1];
        }
        return " -- Invalid Arg for Gender of Person:" + value + " -- ";
    }

    // -----------------------------------------------------------------------------------
    // A response was selected by either clicking, or one of the dialog buttons.
    public void ResponseClicked(int responseID)
    {
        int resID = responseID;
        if (currentPage > 0)
        {
            resID += (currentPage -1); // subtract one for the previous entry.
        }
        DialogNode node = null;
        try
        {
             node = responseNodes[resID];

        } catch (System.Exception)
        {
            Debug.LogError(" Could not find " + resID);
        }
        if (scriptFragments.CallFragment("action", node.id) == false)
        {
            EndDialog();
            return;
        }
        List<DialogNode> counterResponses = FilterDialogNodes(responseNodes[resID].GetResponseNodes());
        if (counterResponses.Count == 0)
        {
            EndDialog();
            return;
        }

        DisplayDialog(counterResponses[0]); // always choose the first if there happens to be multiple
    }
    public void PrevPageClicked()
    {
        currentPage -= 4;
        DisplayResponses(currentPage);
    }
    public void NextPageClicked()
    {
        currentPage += 4;
        DisplayResponses(currentPage);
    }

}
