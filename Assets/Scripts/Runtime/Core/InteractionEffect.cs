using System;
using UnityEngine;

namespace Caineng.Playground.Core
{
public readonly struct InteractionEffect : IEquatable<InteractionEffect>
{
    public InteractionEffectKind Kind { get; }
    public float Magnitude { get; }
    public float Duration { get; }
    public Vector3 Direction { get; }
    public string SourceId { get; }

    public InteractionEffect(
        InteractionEffectKind kind,
        float magnitude,
        float duration,
        Vector3 direction,
        string sourceId)
    {
        Kind = kind;
        Magnitude = magnitude;
        Duration = duration;
        Direction = direction;
        SourceId = sourceId ?? string.Empty;
    }

    public bool Equals(InteractionEffect other)
    {
        return Kind == other.Kind
            && Mathf.Approximately(Magnitude, other.Magnitude)
            && Mathf.Approximately(Duration, other.Duration)
            && Direction == other.Direction
            && SourceId == other.SourceId;
    }

    public override bool Equals(object obj) => obj is InteractionEffect other && Equals(other);

    public override int GetHashCode() => HashCode.Combine(Kind, Magnitude, Duration, Direction, SourceId);
}
}
