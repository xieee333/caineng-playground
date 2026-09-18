using Caineng.Playground.Core;
using Caineng.Playground.Surfaces;
using NUnit.Framework;
using UnityEngine;

namespace Caineng.Playground.Tests
{
public sealed class PaintableSurfaceGridTests
{
    [Test]
    public void SprayChangesTheNearestCell()
    {
        var grid = new PaintableSurfaceGrid(4, 4, 1f, Vector3.zero);

        var changed = grid.ApplySpray(new Vector3(1.2f, 0f, 2.1f), SurfaceKind.Bounce);

        Assert.That(changed, Is.True);
        Assert.That(grid.GetCell(1, 2).Kind, Is.EqualTo(SurfaceKind.Bounce));
    }

    [Test]
    public void SprayOutsideGridDoesNotThrowOrChangeCells()
    {
        var grid = new PaintableSurfaceGrid(4, 4, 1f, Vector3.zero);

        var changed = grid.ApplySpray(new Vector3(-1f, 0f, 2f), SurfaceKind.Liquid);

        Assert.That(changed, Is.False);
        Assert.That(grid.GetCell(0, 2).Kind, Is.EqualTo(SurfaceKind.None));
    }
}
}
