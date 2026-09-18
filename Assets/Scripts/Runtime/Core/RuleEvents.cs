using System.Collections.Generic;

namespace Caineng.Playground.Core
{
public interface IRuleEventSink
{
    void Publish(ScoreEvent scoreEvent);

    void Publish(InteractionEffect effect);
}

public sealed class RecordingRuleEventSink : IRuleEventSink
{
    private readonly List<ScoreEvent> scoreEvents = new();
    private readonly List<InteractionEffect> interactionEffects = new();

    public IReadOnlyList<ScoreEvent> ScoreEvents => scoreEvents;
    public IReadOnlyList<InteractionEffect> InteractionEffects => interactionEffects;

    public void Publish(ScoreEvent scoreEvent) => scoreEvents.Add(scoreEvent);

    public void Publish(InteractionEffect effect) => interactionEffects.Add(effect);
}
}
