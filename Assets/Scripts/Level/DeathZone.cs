using UnityEngine;

namespace EverySingleDay.Level
{
    /// <summary>
    /// Invisible kill volume placed below the level. Falling into it costs a life
    /// and respawns the player at the last checkpoint.
    /// </summary>
    [RequireComponent(typeof(Collider2D))]
    public class DeathZone : MonoBehaviour
    {
        private void Awake() => GetComponent<Collider2D>().isTrigger = true;

        private void OnTriggerEnter2D(Collider2D other)
        {
            if (!other.CompareTag("Player")) return;
            other.GetComponent<Player.PlayerHealth>()?.Kill();
        }
    }
}
