using UnityEngine;

namespace EverySingleDay.Collectibles
{
    public enum CollectibleType { Coin, Gem, Health, ExtraLife }

    /// <summary>
    /// A pickup the player collects on trigger contact. Handles a little idle
    /// bob/spin for juice, awards score/health/lives, plays a sound and an
    /// optional particle burst.
    /// </summary>
    [RequireComponent(typeof(Collider2D))]
    public class Collectible : MonoBehaviour
    {
        [Header("Type & Value")]
        public CollectibleType type = CollectibleType.Coin;
        public int value = 10;

        [Header("Idle Animation")]
        public bool bob = true;
        public float bobHeight = 0.15f;
        public float bobSpeed = 2.5f;
        public bool spin = false;
        public float spinSpeed = 90f;

        [Header("Feedback")]
        public GameObject collectEffect;
        public AudioClip collectClip;

        private Vector3 _startPos;
        private float _phase;
        private bool _collected;

        private void Awake()
        {
            GetComponent<Collider2D>().isTrigger = true;
            _startPos = transform.position;
            _phase = Random.Range(0f, Mathf.PI * 2f); // desync grouped pickups
        }

        private void Update()
        {
            if (bob)
            {
                float y = Mathf.Sin(Time.time * bobSpeed + _phase) * bobHeight;
                transform.position = _startPos + Vector3.up * y;
            }
            if (spin)
                transform.Rotate(0f, spinSpeed * Time.deltaTime, 0f);
        }

        private void OnTriggerEnter2D(Collider2D other)
        {
            if (_collected || !other.CompareTag("Player")) return;
            Collect(other.gameObject);
        }

        private void Collect(GameObject player)
        {
            _collected = true;

            switch (type)
            {
                case CollectibleType.Coin:
                case CollectibleType.Gem:
                    Systems.GameManager.Instance?.AddScore(value);
                    if (type == CollectibleType.Coin)
                        Systems.GameManager.Instance?.AddCoin();
                    break;
                case CollectibleType.Health:
                    player.GetComponent<Player.PlayerHealth>()?.Heal(value);
                    break;
                case CollectibleType.ExtraLife:
                    Systems.GameManager.Instance?.AddLife();
                    break;
            }

            var clip = collectClip != null ? collectClip
                     : type == CollectibleType.Gem ? Systems.SfxLibrary.Gem
                     : Systems.SfxLibrary.Coin;
            Systems.AudioManager.Play(clip);
            if (collectEffect != null)
                Instantiate(collectEffect, transform.position, Quaternion.identity);

            Destroy(gameObject);
        }
    }
}
