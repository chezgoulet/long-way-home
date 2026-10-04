/* Desktop implementations for symbols the SP bridge expects but that only exist in the Android
 * build's platform layer. Each one is here because its Android counterpart lives inside an
 * `#ifdef __ANDROID__` block -- so the honest desktop behaviour is stated, not guessed.
 */

/* Called from SP_DropHM_Activate() when a level is entered. On Android it releases a sticky
 * crouch-toggle held by the on-screen touch layout, which exists only there; a desktop has no
 * touch controls and therefore nothing sticky to release. A no-op is the desktop's correct
 * behaviour, not a stub standing in for missing work. */
void IN_ClearCrouchToggle(void)
{
}
