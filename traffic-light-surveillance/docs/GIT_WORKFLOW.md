# Git Workflow, Branching and Commit Plan

## Branching
```
main      stable, one tag per stage (stage-1 … stage-6, v1.0)
develop   integration branch
feature/* one branch per task, merged into develop with a pull request
```

## Commit messages (Conventional Commits)
`feat:`, `fix:`, `docs:`, `test:`, `refactor:`, `build:`, `ci:` — e.g. `feat(driver): add poll support`.

## First push
```bash
cd traffic-light-surveillance
git init -b main
git add .
git commit -m "chore: import traffic light surveillance project"
git remote add origin git@github.com:<you>/traffic-light-surveillance.git
git push -u origin main
```

## Suggested per-stage commits and tags
Make real commits as you work through each stage; at the end of each stage tag it:

| Stage | Typical commits | Tag |
|---|---|---|
| 1 | `docs: add project introduction` | `git tag -a stage-1 -m "Stage 1: introduction"` |
| 2 | `docs: add PRD with FR/NFR`, `docs: add development plan` | `stage-2` |
| 3 | `docs: add architecture and UML`, `build: add Makefiles`, `ci: add workflow`, `docs: add git workflow` | `stage-3` |
| 4 | `feat: add shared uapi header`, `feat: add state machine`, `feat: add sim device`, `feat(driver): add tlight module`, `feat: add tlightd/tlightctl` | `stage-4` |
| 5 | `test: add unit tests`, `test: add integration script`, `fix: …` (each bug), `refactor: …` | `stage-5` |
| 6 | `docs: add final report`, `docs: update README`, release | `stage-6`, `v1.0` |

Push tags with `git push --tags`. Create a GitHub *Release* from `v1.0`.

## Progress tracking
GitHub Issues per requirement (FR-x / NFR-x) and a Project board. Reference issues in commits (`fixes #7`).
