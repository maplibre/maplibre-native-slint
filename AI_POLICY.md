# AI Policy

Follow
[MapLibre's AI Policy](https://github.com/maplibre/maplibre/blob/main/AI_POLICY.md)
and the guidance below for this repository.

## Stay in the loop

Before you mark a pull request ready for review:

- Read every line you are asking maintainers to merge.
- Be able to explain the change, how it fits the codebase, and how you
  validated it, without leaning on the tool to answer review questions.

Design the change. Use AI to draft, explore, or speed up typing, not to replace
understanding the problem or the existing code.

This repository combines MapLibre Native, Slint, and a graphics backend, and
most of the hard parts live where those meet. Context ownership, run loops, and
render timing are easy to get plausibly wrong and hard to catch by reading a
diff. Say which platform and render backend you actually ran, and what you only
reasoned about.

## Pull request descriptions

MapLibre's policy puts it this way:

> To ensure sufficient self review and understanding of the work, it is
> strongly recommended that **contributors write PR descriptions themselves**
> (if needed, using tools for translation or copy-editing).

Writing it yourself is the recommended way there. If you write it with an
assistant instead, the exchange has to do the same work: the assistant asks
until both sides understand the change, and the description is what came out of
that. The contributor supplies the motivation, the alternatives that were
dropped, and the open questions. The assistant reads the diff and names the
decisions in it that the contributor has not looked at closely. An assistant
that was told nothing has nothing to write, and a contributor who was asked
nothing has reviewed nothing.

The [`co-write-pr`](.agents/skills/co-write-pr/SKILL.md) skill in this
repository describes that exchange. Point your assistant at it.

However the description is written, it should explain the motivation, the
approach, the impact, and anything you are unsure about, to the same extent as
one written without any assistance. Two things that follow:

- Say what the diff cannot. Why the change exists, what was tried and dropped,
  what a reader outside this repository will notice.
- Leave out what the diff already says. A file-by-file walkthrough is not
  thoroughness; it costs a reviewer the time they needed for the rest.

## Talk to maintainers in your own voice

Issues, PR bodies, and review replies are a conversation between humans.

- State problems and proposals in your own words, or in words an assistant
  drew out of you and you have read back.
- When a maintainer asks a question, answer from your understanding. Do not
  paste model output as your reply.
- Trim verbosity. Say what matters for the review.

If you used AI to polish English, read the result once and adjust it so it
sounds like you. For translation, writing in your native language and adding an
English version in a quote block works well.

## When AI context belongs in a thread

Sometimes a snippet from an AI session helps reviewers, for example a design
option you rejected. Share it in a way that keeps the thread readable:

- **A few lines:** Put them in a quote block (`>`), label them as AI-generated,
  and add a short note in your own words on why they are relevant.
- **More than a few lines:** Put the full text in a
  [GitHub Gist](https://gist.github.com/) (or similar) and link it. In the
  comment, summarize in your own words what the gist contains and what you want
  reviewers to take from it.

Do not post long, unedited AI output in issues or pull requests without
maintainer approval.

## Disclosure

When disclosure applies under MapLibre's policy, fill in the **AI assistance**
section of the pull request template. Say which tools were used and what each
side contributed. Disclosure is not penalized.

## Credits

Practices in this document were informed by the AI policies of
[maplibre-native-ffi](https://github.com/maplibre/maplibre-native-ffi/blob/main/AI_POLICY.md),
[uv](https://github.com/astral-sh/.github/blob/main/AI_POLICY.md),
[ripgrep](https://github.com/BurntSushi/ripgrep/blob/master/AI_POLICY.md), and
[Ghostty](https://github.com/ghostty-org/ghostty/blob/main/AI_POLICY.md).
