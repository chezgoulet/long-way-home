# Evidence — allegiance weights a mark and bends an order

**Session 2026-11-08.** Stage D. `docs/affinities-and-allegiance.md` says an allegiance is not a team
but **a weighting on two things**: how an event lands, and whether an order is carried out and how.
The record already carried `Allegiance`; it previously weighted only a meeting's **stance**. This
slice builds the other two uses, where the document puts them: the mark writer and the dispatch.

Save format remains **66**: no new stored field. The event's political reading is not saved; it is
an argument to a writer that already existed, and the order's reading is a property of the order
class.

---

## What was built

**How an event lands.** `AllegianceWeight(c, flavour)` is the signed weight a person's declared side
gives an event that reads as Starfleet procedure or Maquis pragmatism: positive when the reading is
theirs, negative when it is the other side's, zero for a neutral reading or for Delta native,
civilian and Borg-recovered allegiances, which have no dog in the Starfleet/Maquis fight `[our call]`.
`EffectiveValence` adds it at the one mark writer, `Remember`, which now takes the event's flavour.
The daily bond fold is unchanged: it reads whatever valence the mark carries.

The meeting's decision is the event this reaches in the simulation. `ApplyMeetingOutcome` ->
`PublishMeetingReport` writes the decision into the room (`MEM_SAW`) and into the readers outside it
(`MEM_LOG`) with the chosen option's own flavour, so the same decision lands positive on a
Starfleet-weighted participant and negative on a Maquis-weighted one.

**Whether the order is carried out, and how.** `OrderFlavour(order)` gives each order class the same
axis. `DispatchOrder` reads the recipient's weight:

- a strong weight **against** the order's reading (`<= -0.30`) makes them resent it, and the answer is
  the bend: *"Aye. My way, not the book's."* The order is still applied -- the design is explicit
  that they do not refuse to exist -- and the cost is a `MEM_ORDER` mark toward the giver, whose
  valence also carries the order's reading.
- a strong weight **for** it (`>= +0.30`) answers *"Aye -- that is the right call."*
- neutral or weak leaves the morale/condition/trait rule exactly as it was.

**The mapping is ours `[our call]`,** because canon gives no table of which order is "Starfleet". The
document's two poles are procedure and making do, and each class is put on one or left neutral:
`secure`, `evacuate` and `maintain` read Starfleet procedure; `advance` and `build` read Maquis
pragmatism; `see to` is neutral. It is recorded here so it can be overruled in one place.

## The command

```sh
scripts/stage-d-check.sh allegiance
```

It compiles the ship suite on the host (no engine, no game data), runs the whole suite, prints
`test_ship_core --allegiance`, then falsifies once: an `AllegianceWeight` that weights nothing must
make the same transcript fail.

```text
==> stage D: the ship suite, built here

==> allegiance weights a mark and bends an order (test_ship_core --allegiance)
== the same event, read as Starfleet procedure
  EVENT: base valence +0.00, the same decision to both
  CREW A: Crewman 021 -- allegiance the Maquis (+0.80): weight -0.80, mark -0.80
  CREW B: Crewman 022 -- allegiance Starfleet (+0.80): weight +0.80, mark +0.80
== one order (maintain), one recipient (a the Maquis engineer)
  order reads: Starfleet procedure; allegiance weight -0.85; given 1, refused 0
  it bends: 1; it resents: 1; reply: "Aye. My way, not the book's."
  the mark it costs: a MEM_ORDER mark toward the giver
all demonstrations passed

==> falsification: an allegiance that weights nothing must fail the transcript
    the broken copy fails the transcript; the tree passes

PASS  the same event lands opposite on two declared sides, an opposed order bends and pays
      a mark, and a planted allegiance that weights nothing fails the same transcript
```

The rules are also in the suite (`TestAllegianceValenceAndCompliance`), which is what
`scripts/test.sh` runs.

## PASS lines, and what each is evidence of

- **PASS — the same event lands opposite.** Base valence `+0.00`, read as Starfleet procedure: the
  Maquis-weighted record carries `-0.80`, the Starfleet-weighted record `+0.80`. The explanation is
  the declared allegiance, printed with each mark, and the bond fold reads the mark unchanged.
- **PASS — an opposed order bends and pays.** A high-morale Maquis engineer given the Starfleet-reading
  `maintain` order is not refused: `given 1, refused 0`, `bent 1`, and the reply is the bend. The
  cost is a `MEM_ORDER` mark toward the giver; the note names the allegiance and the reading.
- **PASS — a neutral order is untouched.** `TestOrderRecipientAsPerson`'s warm recipient of `see to`
  (neutral) still carries no mark, which keeps the old behaviour where the document did not ask for
  a change.
- **PASS — the simulation path, not only the writer.** The test drives `ApplyMeetingOutcome` with a
  Starfleet-reading option and checks the Maquis participant's meeting mark is negative.
- **PASS — the guard has teeth.** An `AllegianceWeight` planted to return zero fails the transcript;
  the tree passes.

## What is not verified, and is said plainly

- **The magnitudes are invented `[inv]`.** The document gives the direction and says the strength
  matters; it gives no number for how much a mark should move. The threshold (`0.30`) and the use of
  the declared strength as the weight are this slice's choice, recorded so they can be tuned.
- **The other allegiances are given no weight.** Delta native, civilian and Borg-recovered are real
  sides; the document's engine is the Maquis/Starfleet split, and inventing a political reading for
  the others would be a different design. They take the neutral path.
- **The in-engine meeting/order was not run** — static host, no engine and no game data. The
  console surface is unchanged; the model path is the unit transcript.
