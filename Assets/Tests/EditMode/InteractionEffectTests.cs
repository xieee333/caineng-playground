using Caineng.Playground.Core;
using NUnit.Framework;
using UnityEngine;

namespace Caineng.Playground.Tests
{
public sealed class InteractionEffectTests
{
    [Test]
    public void BubbleEffectHasPositiveDuration()
    {
        var effect = new InteractionEffect(
            InteractionEffectKind.Bubble,
            1f,
            2f,
            Vector3.up,
            "bubble-cannon");

        Assert.That(effect.Duration, Is.GreaterThan(0f));
    }
}
}
