/**
 *  StarHaven Project - CMPT 330
 *  Grant MacEwan University 2015
 *  Members: Robert Babiak, Justyn Huculak, Winston Kouch, Ian Nalbach
 *
 *
 * This code is used for dialog scenes itself. It gets the NPC and the right person
 * based on story data. It can also play NPC animations, set camera view and emotes
 * (emotes not integrated yet)
 * 
 * 
 * Scene Objects		Description
 * 
 * name					The scene name.
 * camera				Camera to be used.
 * pawn_male			Male avatar of NPC.
 * pawn_female			Female avatar of NPC.
 *
 */

using UnityEngine;
using System.Collections;
using System;
using UnityEngine.UI;

[Serializable]
public class SceneItem
{
    public string name;
    public GameObject camera = null;
    public Pawn pawn_male;
    public Pawn pawn_female;

	// Change camera view to specified camera of scene target
    public void SetCamera(Camera cam)
    {
        if (camera != null)
        {
            Debug.Log("Set camera to " + camera.name);
            cam.transform.position = camera.transform.position;
            cam.transform.rotation = camera.transform.rotation;
        }
    }
    // returns the active pawn, male before female, null if both are inactive.
    public Pawn GetPawn()
    {
        if (pawn_male != null && pawn_male.gameObject.activeSelf)
        {
            return pawn_male;
        }
        if (pawn_female != null && pawn_female.gameObject.activeSelf)
        {
            return pawn_female;
        }
        return null;
    }
    public void PlayAnim(PawnEmote anim)
    {
        Pawn p = GetPawn();
        if (p) p.PlayEmoteAnim(anim);
    }

}

[Serializable]
public class DialogScene
{
    public SceneItem[] positions;
    public Pawn getPawn(int person)
    {
        if (person >= 0 && person < positions.Length)
        {
            positions[person].GetPawn();
        }
        return null;
    }

    public Pawn GetPawn(int person)
    {
        if (person >= 0 && person < positions.Length)
        {
            return positions[person].GetPawn();
        }
        return null;
    }

    public void SetCamera(Camera cam, int person)
    {
/*
        float t = 0.0f;
        while (t < 1.0f)
        {
            yield return new WaitForEndOfFrame();
            t = Mathf.Clamp01(t + Time.deltaTime / FadeOutTime);
            DrawQuad(FadeColor, t);
        }
*/
        if (person >= 0 && person < positions.Length)
        {
            positions[person].SetCamera(cam);
        }
    }
    public void PlayAnim(int person, PawnEmote anim)
    {
        if (person >= 0 && person < positions.Length)
        {
            positions[person].PlayAnim(anim);
        }

    }
}
