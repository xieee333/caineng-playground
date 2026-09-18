using Caineng.Playground.Core;
using Caineng.Playground.Surfaces;
using NUnit.Framework;

namespace Caineng.Playground.Tests
{
public sealed class ColorTraversalMotorTests
{
    [Test]
    public void EmptyCellDoesNotGrantTraversalBoost()
    {
        var motor = new ColorTraversalMotor();

        var result = motor.Resolve(SurfaceKind.None, 1f);

        Assert.That(result.SpeedMultiplier, Is.EqualTo(1f));
        Assert.That(result.CanSubmerge, Is.False);
        Assert.That(result.CanWallRide, Is.False);
    }

    [Test]
    public void LiquidSurfaceEnablesFastSubmergeAndWallRide()
    {
        var motor = new ColorTraversalMotor();

        var result = motor.Resolve(SurfaceKind.Liquid, 5f);

        Assert.That(result.SpeedMultiplier, Is.EqualTo(11f).Within(0.001f));
        Assert.That(result.CanSubmerge, Is.True);
        Assert.That(result.CanWallRide, Is.True);
    }

    [Test]
    public void FloatSurfaceReducesGravity()
    {
        var motor = new ColorTraversalMotor();

        var result = motor.Resolve(SurfaceKind.Float, 5f);

        Assert.That(result.SpeedMultiplier, Is.EqualTo(4f).Within(0.001f));
        Assert.That(result.GravityScale, Is.EqualTo(0.25f));
    }
}
}
