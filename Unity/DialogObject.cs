/**
 *  StarHaven Project - CMPT 330
 *  Grant MacEwan University 2015
 *  Members: Robert Babiak, Justyn Huculak, Winston Kouch, Ian Nalbach
 *
 *
 * This code is used for dialog objects.
 * It can be used to change camera focus on scene objects during dialog scenes.
 * Also used to start dialogs in OnInteraction.
 * 
 * 
 * Public Objects		Description
 * 
 * dialogID				Associated dialog ID of story nodes
 * scene				Categorized by our scenes ie. Lord Balanor
 * 
 * Private Objects		Description
 * 
 * dialog				DialogManager object
 *
 */

using UnityEngine;
using System.Collections;
using System;
using UnityEngine.UI;

// -----------------------------------------------------------------------------------
[RequireComponent(typeof(CapsuleCollider))]

[Serializable]
public partial class DialogObject : InteractiveObject
{
    public int dialogID = 0;
    private DialogManager dialog = null;
    protected override void Start ()
    {
        base.Start();
        dialog = GameObject.FindWithTag("GlobalData").GetComponent<DialogManager>();

    }

    // Update is called once per frame
    void Update ()
    {

    }
    protected override bool OnInteraction()
    {
        if (log) Debug.Log("Activate Dialog " + dialogID);
        if (dialogID != 0)
        {
            return dialog.BeginDialog(dialogID);
        }
        return false;
    }
}
