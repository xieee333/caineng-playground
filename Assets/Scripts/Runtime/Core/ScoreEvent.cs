using System;
using UnityEngine;

namespace Caineng.Playground.Core
{
public readonly struct ScoreEvent : IEquatable<ScoreEvent>
{
    public string ActorId { get; }
    public string Reason { get; }
    public int Amount { get; }
    public Vector3 WorldPosition { get; }

    public ScoreEvent(string actorId, string reason, int amount, Vector3 worldPosition)
    {
        ActorId = actorId ?? string.Empty;
        Reason = reason ?? string.Empty;
        Amount = amount;
        WorldPosition = worldPosition;
    }

    public bool Equals(ScoreEvent other)
    {
        return ActorId == other.ActorId
            && Reason == other.Reason
            && Amount == other.Amount
            && WorldPosition == other.WorldPosition;
    }

    public override bool Equals(object obj) => obj is ScoreEvent other && Equals(other);

    public override int GetHashCode() => HashCode.Combine(ActorId, Reason, Amount, WorldPosition);
}
}
