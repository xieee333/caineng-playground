using Caineng.Playground.Core;
using NUnit.Framework;
using UnityEngine;

namespace Caineng.Playground.Tests
{
public sealed class RuleEventsTests
{
    [Test]
    public void ScoreEventsPreserveReasonAndActor()
    {
        var score = new ScoreEvent("p1", "bounce-save", 10, Vector3.zero);

        Assert.That(score.ActorId, Is.EqualTo("p1"));
        Assert.That(score.Reason, Is.EqualTo("bounce-save"));
    }

    [Test]
    public void EventSinkPreservesPublicationOrder()
    {
        var sink = new RecordingRuleEventSink();
        sink.Publish(new ScoreEvent("p1", "spray", 2, Vector3.zero));
        sink.Publish(new InteractionEffect(
            InteractionEffectKind.Knockback,
            1f,
            0.25f,
            Vector3.forward,
            "test"));

        Assert.That(sink.ScoreEvents, Has.Count.EqualTo(1));
        Assert.That(sink.InteractionEffects, Has.Count.EqualTo(1));
    }
}
}
