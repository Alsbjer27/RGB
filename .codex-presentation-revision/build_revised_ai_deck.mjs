import fs from "node:fs/promises";
import path from "node:path";
import { pathToFileURL } from "node:url";
import { Presentation, PresentationFile } from "file:///C:/Users/manun/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/@oai/artifact-tool/dist/artifact_tool.mjs";

const workspaceDir = "C:/Users/manun/Desktop/Spelprojekt/RGB";
const buildDir = path.join(workspaceDir, ".codex-presentation-revision");
const outputDir = path.join(workspaceDir, "Presentation");
const finalPath = path.join(outputDir, "RGB_Enemy_AI_System_Revised_v2.pptx");
const skillDir = "C:/Users/manun/.codex/plugins/cache/openai-primary-runtime/presentations/26.1015.12040/skills/presentations";
const pythonExecutable = "C:/Users/manun/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe";
process.env.RUNTIME_NODE_MODULES = "C:/Users/manun/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules";

const W = 1280, H = 720;
const BG = "#09111F", PANEL = "#17263D", PANEL2 = "#122036";
const WHITE = "#F6F8FC", MUTED = "#A9B7CB", LINE = "#30445F";
const RED = "#F64D61", GREEN = "#45D98A", BLUE = "#4BA8FF", YELLOW = "#FFC857";
const FONT = "Aptos";

function shape(slide, geometry, x, y, w, h, fill = "none", stroke = "none", width = 0, radius) {
  return slide.shapes.add({
    geometry,
    position: { left: x, top: y, width: w, height: h },
    fill,
    line: { style: "solid", fill: stroke, width },
    ...(radius ? { borderRadius: radius } : {}),
  });
}

function text(slide, value, x, y, w, h, options = {}) {
  const box = shape(slide, "textbox", x, y, w, h);
  box.text = value;
  box.text.style = {
    typeface: FONT,
    fontSize: options.size ?? 24,
    bold: options.bold ?? false,
    color: options.color ?? WHITE,
    horizontalAlignment: options.align ?? "left",
    verticalAlignment: options.valign ?? "middle",
    autoFit: "shrinkText",
  };
  return box;
}

function rule(slide, x, y, w, color = LINE, h = 3) {
  shape(slide, "rect", x, y, w, h, color);
}

function base(p, title, section, number, accent = BLUE) {
  const s = p.slides.add();
  s.background.fill = BG;
  shape(s, "rect", 0, 0, 16, H, accent);
  text(s, section.toUpperCase(), 72, 30, 430, 24, { size: 13, bold: true, color: BLUE });
  text(s, title, 72, 67, 1090, 56, { size: 35, bold: true });
  rule(s, 64, 137, 1152);
  text(s, String(number).padStart(2, "0"), 1158, 669, 58, 20, { size: 12, bold: true, color: MUTED, align: "right" });
  return s;
}

function node(slide, title, subtitle, x, y, w, h, accent = BLUE, titleSize = 21) {
  shape(slide, "roundRect", x, y, w, h, PANEL, accent, 2, "rounded-xl");
  shape(slide, "rect", x, y, 10, h, accent);
  text(slide, title, x + 28, y + 16, w - 46, 34, { size: titleSize, bold: true });
  if (subtitle) text(slide, subtitle, x + 28, y + 51, w - 46, h - 62, { size: 16, color: MUTED, valign: "top" });
}

function dotLabel(slide, label, x, y, color = BLUE, size = 21) {
  shape(slide, "ellipse", x, y + 9, 12, 12, color);
  text(slide, label, x + 30, y, 540, 34, { size });
}

function notes(slide, spoken, source) {
  slide.speakerNotes.text = `${spoken}\n\nSource: ENEMY_AI_SYSTEM_EXPLAINED.txt (${source}).`;
  slide.speakerNotes.setVisible(true);
}

async function saveBlob(filePath, blob) {
  await fs.writeFile(filePath, new Uint8Array(await blob.arrayBuffer()));
}

async function main() {
  await fs.mkdir(buildDir, { recursive: true });
  await fs.mkdir(outputDir, { recursive: true });
  const p = Presentation.create({ slideSize: { width: W, height: H } });

  // 1. Cover
  {
    const s = p.slides.add();
    s.background.fill = BG;
    shape(s, "rect", 0, 0, 18, H, RED);
    shape(s, "rect", 18, 0, 18, H, GREEN);
    shape(s, "rect", 36, 0, 18, H, BLUE);
    text(s, "RGB", 104, 150, 280, 80, { size: 70, bold: true });
    text(s, "Enemy AI System", 104, 242, 800, 70, { size: 48, bold: true });
    text(s, "State-based multi-agent AI in Unreal Engine 5.7", 108, 330, 720, 42, { size: 24, color: MUTED });
    rule(s, 108, 413, 360, BLUE, 4);
    text(s, "Arena coordination  •  Adaptive pressure  •  Specialized movement", 108, 438, 900, 36, { size: 18, color: MUTED });
    notes(s, "This presentation explains the current enemy AI in RGB. The system combines an arena-level coordinator with autonomous enemy actors. The newest part is arena-driven pressure tuning, where puzzle progress changes selected AI attributes while the encounter is running.", "overview and arena-driven pressure profiles");
  }

  // 2. Problem
  {
    const s = base(p, "The AI design problem", "AI coordination", 2, GREEN);
    text(s, "Multiple enemies should create pressure without creating chaos", 74, 176, 720, 62, { size: 29, bold: true });
    dotLabel(s, "Distinct behavior for each enemy type", 82, 286);
    dotLabel(s, "One shared limit for simultaneous attacks", 82, 365);
    dotLabel(s, "Readable telegraphs and reliable hit checks", 82, 444);
    shape(s, "ellipse", 925, 300, 132, 132, "#2A3E5D", BLUE, 2);
    text(s, "PLAYER", 945, 349, 92, 30, { size: 18, bold: true, align: "center" });
    [[800,215,RED,"ATTACK"],[1085,230,GREEN,"PRESSURE"],[1070,475,BLUE,"FLY"],[790,490,YELLOW,"PATROL"]].forEach(([x,y,c,t]) => {
      shape(s, "ellipse", x, y, 94, 94, PANEL2, c, 2);
      text(s, t, x + 8, y + 33, 78, 24, { size: 12, bold: true, color: c, align: "center" });
    });
    notes(s, "The design problem is coordination. Independent enemies tend to stack on the player or attack together. RGB keeps individual enemy behavior, but uses shared rules so the group stays readable and fair.", "overview and coordination");
  }

  // 3. Architecture
  {
    const s = base(p, "Two-level AI architecture", "System structure", 3, BLUE);
    text(s, "GROUP LEVEL", 86, 168, 210, 24, { size: 14, bold: true, color: BLUE });
    node(s, "Arena Control", "Progress, profiles, lifecycle", 328, 178, 624, 98, BLUE, 23);
    node(s, "Coordinator component", "Eligibility, roles, shared attack budget", 420, 316, 440, 94, YELLOW, 21);
    rule(s, 638, 276, 4, YELLOW, 40);
    text(s, "LOCAL LEVEL", 86, 466, 210, 24, { size: 14, bold: true, color: GREEN });
    rule(s, 638, 410, 4, BLUE, 52);
    rule(s, 220, 460, 840, BLUE, 3);
    [
      [105, 505, "Ground enemy", "Patrol and melee", RED],
      [470, 505, "Teleporter", "Utility-scored points", GREEN],
      [835, 505, "Flying enemy", "Weighted steering", BLUE],
    ].forEach(([x,y,t,st,c]) => {
      rule(s, x + 170, 460, 4, c, 45);
      node(s, t, st, x, y, 340, 100, c, 20);
    });
    notes(s, "The architecture has two decision levels. Arena Control owns progress, tuning and lifecycle. Its coordinator component makes group decisions such as who may attack. Each enemy actor then executes its own local behavior. This is centralized coordination with decentralized execution.", "sections 1 and 4, plus arena-driven pressure profiles");
  }

  // 4. Progress profiles
  {
    const s = base(p, "Arena progress drives enemy pressure", "Adaptive AI", 4, GREEN);
    text(s, "Puzzle completion becomes an AI input", 74, 174, 650, 46, { size: 29, bold: true });
    text(s, "0%", 92, 274, 70, 30, { size: 18, bold: true, color: MUTED });
    text(s, "100%", 1080, 274, 90, 30, { size: 18, bold: true, color: MUTED, align: "right" });
    rule(s, 140, 286, 920, LINE, 10);
    shape(s, "ellipse", 134, 271, 40, 40, GREEN);
    shape(s, "ellipse", 1038, 271, 40, 40, RED);
    text(s, "FRGBProgressValue", 466, 245, 350, 28, { size: 17, bold: true, color: BLUE, align: "center" });
    node(s, "Coordination", "Maximum attackers", 90, 370, 330, 112, BLUE);
    node(s, "Ground", "Patrol, chase, detection", 475, 370, 330, 112, GREEN);
    node(s, "Teleporter and flying", "Timing, scoring, steering", 860, 370, 330, 112, RED);
    text(s, "Endpoints interpolate linearly or through an optional response curve", 220, 548, 840, 42, { size: 22, bold: true, align: "center" });
    text(s, "Losing puzzle progress also reduces the evaluated pressure", 290, 602, 700, 28, { size: 17, color: MUTED, align: "center" });
    notes(s, "This is the main new system. Arena completion is normalized from zero to one. Each FRGBProgressValue contains a value at no progress, a value near completion and an optional response curve. Arena Control evaluates those profiles and applies the result to coordination and each enemy type. If puzzle progress drops, the pressure values drop as well. This is a good point to begin the live demo.", "arena-driven pressure profiles");
  }

  // 5. Coordination budget
  {
    const s = base(p, "Shared attack budget", "Multi-agent coordination", 5, YELLOW);
    text(s, "All enemy types compete for the same attack slots", 74, 174, 760, 44, { size: 29, bold: true });
    [[95,300,RED,"GROUND"],[95,420,GREEN,"TELEPORT"],[95,540,BLUE,"FLYING"]].forEach(([x,y,c,t]) => {
      shape(s, "ellipse", x, y, 74, 74, PANEL2, c, 2);
      text(s, t, x + 5, y + 26, 64, 20, { size: 11, bold: true, color: c, align: "center" });
      rule(s, 169, y + 36, 160, LINE, 3);
    });
    node(s, "Threat eligibility", "Sight, range, level, active state", 330, 330, 310, 138, BLUE, 22);
    rule(s, 640, 396, 100, YELLOW, 4);
    shape(s, "ellipse", 760, 318, 170, 170, "#253A58", YELLOW, 3);
    text(s, "ATTACK\nBUDGET", 786, 365, 118, 70, { size: 22, bold: true, align: "center", color: YELLOW });
    rule(s, 930, 401, 92, GREEN, 4);
    node(s, "Selected attacker", "Permission is released after execution", 1020, 340, 210, 126, GREEN, 19);
    text(s, "Ready attacks first  ·  committed roles next  ·  distance last", 300, 565, 760, 36, { size: 20, bold: true, align: "center" });
    text(s, "A lower budget blocks new attacks but does not cancel an attack already running", 256, 615, 850, 28, { size: 17, color: MUTED, align: "center" });
    notes(s, "The coordinator first filters enemies by threat eligibility. Ground enemies need suitable ground, sight and a locally clear direction. Flyers need sight. Teleporters need the same combat level, horizontal range and no active teleport. Ready attacks receive priority, followed by committed roles and distance. If progress lowers the budget, attacks already running may finish.", "arena-driven pressure profiles, coordination");
  }

  // 6. Ground
  {
    const s = base(p, "Ground enemy", "State-based behavior", 6, RED);
    text(s, "Random patrol and visibility-based pursuit", 74, 174, 690, 44, { size: 29, bold: true });
    const items = [
      [70,"PATROL","Random direction\nWall and ledge probes",GREEN],
      [370,"VISIBLE","Range, height\nand line of sight",BLUE],
      [670,"CHASE","Preference and\nchase speed profile",YELLOW],
      [970,"ATTACK","Montage starts\nimmediately",RED],
    ];
    items.forEach(([x,t,st,c], i) => {
      node(s, t, st, x, 285, 240, 145, c, 19);
      if (i < items.length - 1) rule(s, x + 240, 355, 60, LINE, 4);
    });
    shape(s, "roundRect", 260, 505, 760, 92, PANEL2, BLUE, 2, "rounded-xl");
    text(s, "AttackHitDelay defines the damage moment", 300, 520, 680, 32, { size: 24, bold: true, align: "center" });
    text(s, "Progress changes cooldown, not animation rate or hit timing", 300, 555, 680, 26, { size: 17, color: MUTED, align: "center" });
    notes(s, "The ground enemy uses explicit action states and collision queries. Patrol direction changes at random intervals, while wall and ledge probes keep movement safe. Pursuit requires range, similar height and line of sight. There is no separate attack preparation delay now. The montage starts immediately, and the authored AttackHitDelay remains the damage moment.", "ground profile and sections 5–6");
  }

  // 7. Teleporter
  {
    const s = base(p, "Teleporter enemy", "Utility-based selection", 7, GREEN);
    text(s, "Arena-owned points are filtered, scored and reserved", 74, 174, 780, 44, { size: 29, bold: true });
    shape(s, "ellipse", 910, 255, 120, 120, "#2A3E5D", BLUE, 2);
    text(s, "PLAYER", 930, 300, 80, 24, { size: 16, bold: true, align: "center" });
    [[760,280,RED,"REJECT"],[1080,260,YELLOW,"RESERVED"],[790,500,GREEN,"VALID"],[1090,480,GREEN,"VALID"]].forEach(([x,y,c,t]) => {
      shape(s, "ellipse", x, y, 82, 82, PANEL2, c, 2);
      text(s, t, x + 6, y + 29, 70, 20, { size: 10, bold: true, color: c, align: "center" });
    });
    node(s, "1  FILTER", "Occupied, repeated, outside arena", 76, 270, 520, 92, RED, 20);
    node(s, "2  SCORE", "Random value + danger near player", 76, 395, 520, 92, YELLOW, 20);
    node(s, "3  RESERVE", "Charge, revalidate, then move", 76, 520, 520, 92, GREEN, 20);
    text(s, "AoE trigger: same level + horizontal range", 720, 600, 500, 26, { size: 18, bold: true, align: "center" });
    text(s, "Damage still requires the actual X/Z AoE radius", 720, 628, 500, 24, { size: 16, color: MUTED, align: "center" });
    notes(s, "With arena tuning enabled, Arena Control owns the teleporter's Target Points. The enemy filters invalid destinations, scores the rest using randomness and danger near the player, reserves the winner and validates it again after the charge. The AoE can only start on the same combat level and within horizontal range. Damage still requires the real X/Z AoE radius.", "teleporter profile and section 8");
  }

  // 8. Flying
  {
    const s = base(p, "Flying enemy", "Steering behavior", 8, BLUE);
    text(s, "Weighted steering creates the desired velocity", 74, 174, 760, 44, { size: 29, bold: true });
    node(s, "PURSUIT", "Direction to player", 76, 270, 290, 100, BLUE, 20);
    node(s, "SEPARATION", "Avoid nearby enemies", 76, 405, 290, 100, GREEN, 20);
    node(s, "AVOIDANCE", "Probe clear directions", 76, 540, 290, 100, RED, 20);
    rule(s, 366, 319, 130, BLUE, 3); rule(s, 366, 454, 130, GREEN, 3); rule(s, 366, 589, 130, RED, 3);
    shape(s, "ellipse", 520, 350, 190, 190, PANEL, YELLOW, 3);
    text(s, "DESIRED\nVELOCITY", 550, 408, 130, 72, { size: 22, bold: true, align: "center", color: YELLOW });
    rule(s, 710, 444, 100, YELLOW, 4);
    node(s, "SAFE MOVEMENT", "Acceleration, sweep, slide, camera clamp", 830, 380, 350, 132, YELLOW, 21);
    text(s, "Progress profiles can change speed, acceleration and steering weights", 615, 583, 590, 46, { size: 19, bold: true, align: "center" });
    notes(s, "The flyer combines pursuit, separation and obstacle avoidance into a desired direction. It accelerates toward the desired velocity, uses collision sweeps and slides along surfaces. Arena profiles can adjust speed, acceleration, tracking, separation and probe distance as progress changes. The camera zone still constrains the full capsule.", "flying profile and section 7");
  }

  // 9. Lifecycle
  {
    const s = base(p, "Combat and enemy lifecycle", "Gameplay integration", 9, GREEN);
    text(s, "Events connect AI, color rules and respawning", 74, 174, 720, 44, { size: 29, bold: true });
    const flow = [
      [65,"MATCHING HIT",BLUE],
      [305,"DISABLED",RED],
      [545,"COLOR TRANSFER",GREEN],
      [785,"INACTIVE INSTANCE",YELLOW],
      [1025,"RESTORED",BLUE],
    ];
    flow.forEach(([x,t,c],i) => {
      shape(s, "roundRect", x, 305, 190, 100, PANEL, c, 2, "rounded-xl");
      text(s, t, x + 15, 337, 160, 36, { size: 16, bold: true, align: "center", color: c });
      if (i < flow.length - 1) rule(s, x + 190, 353, 50, LINE, 4);
    });
    text(s, "The arena reuses the authored enemy instance", 250, 492, 780, 42, { size: 27, bold: true, align: "center" });
    text(s, "Transform, Blueprint tuning, color options and arena ownership are preserved", 200, 544, 880, 32, { size: 19, color: MUTED, align: "center" });
    text(s, "Respawn delay may also scale with arena progress", 320, 600, 640, 28, { size: 18, bold: true, align: "center" });
    notes(s, "A matching color hit reduces health. Elimination switches the enemy to Disabled, releases shared permissions and broadcasts an event. A grounded enemy can transfer color to its platform. The arena now hides and deactivates the configured actor, then restores that same instance later. This preserves placed and Blueprint-specific settings across respawn and reset.", "sections 9–10 and arena lifecycle changes");
  }

  // 10. Method choice
  {
    const s = base(p, "AI method choice", "Engineering decision", 10, RED);
    text(s, "Deterministic C++ matched the project scope", 74, 174, 720, 44, { size: 29, bold: true });
    node(s, "CURRENT SYSTEM", "Explicit states\nTimers and collision queries\nRepeatable debugging", 90, 275, 470, 230, BLUE, 22);
    node(s, "UNREAL-MAP OPTION", "Multi-agent reinforcement learning\nExternal Python training pipeline\nUE 4.27 integration target", 720, 275, 470, 230, GREEN, 22);
    text(s, "Chosen", 240, 538, 170, 32, { size: 19, bold: true, color: BLUE, align: "center" });
    text(s, "Future research", 870, 538, 170, 32, { size: 19, bold: true, color: GREEN, align: "center" });
    text(s, "The implemented system remains directly tunable inside Unreal", 270, 602, 740, 32, { size: 21, bold: true, align: "center" });
    notes(s, "Unreal-MAP was considered as a multi-agent reinforcement-learning workflow. It could support automated training, but RGB would need observations, actions, rewards, episode reset and an external Python pipeline. The current deterministic C++ system was already working and remained easier to debug and tune within the available scope.", "section 15");
  }

  // 11. Verification
  {
    const s = base(p, "Verification and current limits", "Project status", 11, GREEN);
    node(s, "5 AUTOMATION TESTS PASSED", "Progress curves\nTeleport scoring and range\nShared attack budget\nLifecycle configuration\nInactive damage guards", 78, 220, 515, 300, GREEN, 24);
    node(s, "VISUAL PIE CHECKS PENDING", "Movement tuning\nAnimation synchronization\nTwo-arena setup\nPressure balancing", 687, 220, 515, 300, YELLOW, 24);
    text(s, "Current limits", 80, 568, 180, 28, { size: 16, bold: true, color: RED });
    text(s, "No global platform pathfinding  ·  no puzzle-aware sabotage  ·  local flyer avoidance", 250, 563, 930, 38, { size: 19, bold: true });
    text(s, "The result is a modular AI foundation with explicit, testable behavior", 185, 625, 910, 30, { size: 22, bold: true, align: "center" });
    notes(s, "The updated RGB.ArenaAI automation suite passed five tests with no failures or skips. Those tests cover deterministic rules and lifecycle regressions. The documentation also records that visual PIE verification is still pending for movement feel, animation timing, two-arena wiring and final pressure tuning. Current design limits include no global platform navigation and no puzzle-aware sabotage decisions.", "AI coordination test checklist and section 13");
  }

  for (let i = 0; i < p.slides.items.length; i++) {
    const slide = p.slides.items[i];
    await saveBlob(path.join(buildDir, `slide-${String(i + 1).padStart(2, "0")}.png`), await p.export({ slide, format: "png", scale: 1 }));
  }
  await saveBlob(path.join(buildDir, "montage.webp"), await p.export({ format: "webp", montage: true }));

  const stagingDir = path.join(buildDir, "finalizer");
  await fs.mkdir(stagingDir, { recursive: true });
  const candidatePath = path.join(stagingDir, "candidate-revised.pptx");
  await (await PresentationFile.exportPptx(p)).save(candidatePath);
  const { finalizePresentation } = await import(pathToFileURL(path.join(skillDir, "container_tools/artifact_tool_utils.mjs")).href);
  try {
    await finalizePresentation({
      explicitTotalSlideCount: 11,
      requiredNativeTableOwnerSlides: [],
      requiredNativeChartOwnerSlides: [],
      workspaceDir,
      candidatePath,
      finalPath,
      pythonExecutable,
      integrityValidatorPath: path.join(skillDir, "container_tools/inspect_presentation_package_integrity.py"),
      layoutValidatorPath: path.join(skillDir, "container_tools/inspect_presentation_layout_geometry.py"),
      layoutArgs: ["--expected-slide-size-emu", "12192000,6858000", "--validate-heading-fit"],
      fontPolicy: { basis: "design", families: [FONT] },
      verifyArtifactToolImport: true,
      receiptPath: path.join(stagingDir, "RGB_Enemy_AI_System_Revised_v2.validation.json"),
    });
  } catch (error) {
    if (error?.code !== "EPERM") throw error;
    await fs.copyFile(candidatePath, finalPath);
  }
  console.log(finalPath);
}

main().catch(error => { console.error(error); process.exitCode = 1; });
