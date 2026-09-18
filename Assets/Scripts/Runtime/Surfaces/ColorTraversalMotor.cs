using Caineng.Playground.Core;

namespace Caineng.Playground.Surfaces
{
public readonly struct TraversalResult
{
    public float SpeedMultiplier { get; }
    public bool CanSubmerge { get; }
    public bool CanWallRide { get; }
    public float GravityScale { get; }

    public TraversalResult(
        float speedMultiplier,
        bool canSubmerge,
        bool canWallRide,
        float gravityScale = 1f)
    {
        SpeedMultiplier = speedMultiplier;
        CanSubmerge = canSubmerge;
        CanWallRide = canWallRide;
        GravityScale = gravityScale;
    }
}

public sealed class ColorTraversalMotor
{
    public TraversalResult Resolve(SurfaceKind kind, float baseSpeed)
    {
        return kind switch
        {
            SurfaceKind.Liquid => new TraversalResult(baseSpeed * 2.2f, true, true),
            SurfaceKind.Bounce => new TraversalResult(baseSpeed * 1.1f, false, false),
            SurfaceKind.Float => new TraversalResult(baseSpeed * 0.8f, false, false, 0.25f),
            _ => new TraversalResult(baseSpeed, false, false)
        };
    }
}
}
