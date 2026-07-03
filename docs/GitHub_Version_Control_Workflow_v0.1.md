# GitHub Version Control Workflow v0.1

## 1. Purpose

This document defines the recommended GitHub version control workflow for the FutureWorld Unreal Engine 5 project.

The goals are:

- Keep source code, configuration, design docs, and UE project metadata versioned.
- Store large Unreal assets through Git LFS.
- Avoid committing generated build, cache, and local editor files.
- Keep the main branch stable while allowing feature work to happen safely in branches.
- Make it easy to restore, review, and collaborate on the project over time.

## 2. Repository Strategy

Recommended repository name:

- `FutureWorld`

Recommended default branch:

- `main`

Recommended protected branches:

- `main`
- `release/*` when release builds begin

Recommended branch types:

- `feature/<short-name>` for new gameplay systems or content.
- `fix/<short-name>` for bug fixes.
- `docs/<short-name>` for documentation-only changes.
- `prototype/<short-name>` for experimental work that may be discarded.
- `release/<version>` for release stabilization.

Examples:

- `feature/player-movement`
- `feature/vehicle-entry-exit`
- `fix/core-vehicle-life-penalty`
- `docs/github-workflow`

## 3. What Should Be Committed

Commit these:

- `Source/`
- `Config/`
- `Content/`
- `Plugins/` when project-owned plugins are added
- `docs/`
- `*.uproject`
- `*.uplugin`
- `.gitignore`
- `.gitattributes`
- build scripts and CI configuration

Do not commit these:

- `Binaries/`
- `Build/`
- `DerivedDataCache/`
- `Intermediate/`
- `Saved/`
- `.vs/`
- IDE user settings
- packaged builds
- local `.env` files

## 4. Git LFS Policy

Unreal assets and large media files should be tracked with Git LFS.

The included `.gitattributes` tracks common binary asset formats, including:

- `.uasset`
- `.umap`
- `.fbx`
- `.png`
- `.jpg`
- `.wav`
- `.mp4`

Before the first commit on a new machine, install Git LFS:

```powershell
git lfs install
```

Check tracked LFS patterns:

```powershell
git lfs track
```

## 5. Initial GitHub Setup

Run these commands from the project root after Git is installed:

```powershell
git init
git lfs install
git add .gitignore .gitattributes docs
git commit -m "Initialize GitHub version control workflow"
git branch -M main
git remote add origin https://github.com/<owner>/FutureWorld.git
git push -u origin main
```

If the Unreal project files already exist at that time, add them in the same first commit:

```powershell
git add FutureWorld.uproject Source Config Content Plugins docs .gitignore .gitattributes
git commit -m "Initialize FutureWorld UE5 project"
```

## 6. Daily Development Workflow

1. Update local `main`.

```powershell
git checkout main
git pull --ff-only
```

2. Create a focused branch.

```powershell
git checkout -b feature/player-movement
```

3. Work in small, reviewable steps.

```powershell
git status
git add <changed-files>
git commit -m "Add player movement input skeleton"
```

4. Push the branch.

```powershell
git push -u origin feature/player-movement
```

5. Open a pull request on GitHub.

6. Merge only after review and basic verification.

## 7. Commit Message Style

Use short imperative messages:

- `Add player movement input skeleton`
- `Create vehicle base class`
- `Document GitHub version control workflow`
- `Fix respawn rule default values`

For larger changes, use a body:

```text
Add vehicle entry interaction flow

- Adds base vehicle enter/exit API
- Documents possession handoff expectations
- Leaves animation hookup for Blueprint phase
```

## 8. Pull Request Checklist

Before merging a pull request:

- The branch has a clear purpose.
- Generated files are not included.
- Large binary files are tracked through Git LFS.
- C++ compiles locally when code is present.
- New gameplay behavior is tested in the smallest relevant map.
- Documentation is updated when architecture or workflow changes.

## 9. Unreal Asset Collaboration Rules

Unreal binary assets cannot be merged like text files. To reduce conflicts:

- Avoid having multiple people edit the same `.umap` at the same time.
- Prefer small test maps for prototype work.
- Split reusable gameplay configuration into Data Assets.
- Communicate before editing shared core Blueprints.
- Use clear PR descriptions when changing maps, Data Assets, or Blueprints.

When the team grows, consider Git LFS file locking for high-conflict assets:

```powershell
git lfs lock Content/FutureWorld/Maps/OpenWorld/L_Main.umap
git lfs unlock Content/FutureWorld/Maps/OpenWorld/L_Main.umap
```

## 10. Release Tags

Use tags for playable milestones:

```powershell
git tag -a v0.1.0-prototype -m "First playable movement prototype"
git push origin v0.1.0-prototype
```

Suggested milestone tags:

- `v0.1.0-prototype`
- `v0.2.0-weapon-prototype`
- `v0.3.0-vehicle-prototype`
- `v0.4.0-ai-prototype`
- `v1.0.0-mvp`

## 11. Recommended GitHub Settings

Enable these in GitHub repository settings:

- Require pull request before merging into `main`.
- Require branch to be up to date before merging.
- Use squash merge for feature branches.
- Disable force pushes to `main`.
- Delete merged branches automatically.
- Enable secret scanning.

Optional later:

- GitHub Actions build checks.
- Git LFS file locking.
- Release pages for packaged builds.
- Issue templates for bugs, tasks, and design decisions.

## 12. Next Steps

Immediate next steps:

1. Install Git and Git LFS on the development machine.
2. Create the GitHub repository.
3. Initialize the local repository.
4. Commit `.gitignore`, `.gitattributes`, and this workflow document.
5. Push `main` to GitHub.
6. Create the first feature branch for UE5 project setup.

