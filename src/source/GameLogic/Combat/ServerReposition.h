#pragma once

class CHARACTER;

// How a character reacts when the server moves it to another tile (knockback, lightning push,
// the basic weapon skills' repositioning): packet 0x15 in ReceiveMovePosition.
namespace GameLogic::Combat
{
// True when the character keeps its current animation after the server moved it: it was not
// walking and is in the middle of an attack, skill or other action. A walking, standing, sitting
// or dead character is set to its stand pose as before.
bool KeepsActionOnReposition(const CHARACTER* c, bool wasWalking);

// True when the hero keeps attacking while it slides to a server-set position: auto-attack has a
// target, or the right mouse button is held for a skill (Evil Spirit, Triple Shot...).
bool KeepsAttackingDuringSlide(bool hasAutoAttackTarget, bool isRightButtonHeld);
} // namespace GameLogic::Combat
