import fs from "node:fs/promises";
import path from "node:path";
import { pathToFileURL } from "node:url";
import { Presentation, PresentationFile } from "file:///C:/Users/manun/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/@oai/artifact-tool/dist/artifact_tool.mjs";

const workspaceDir = "C:/Users/manun/Desktop/Spelprojekt/RGB";
const buildDir = path.join(workspaceDir, ".codex-presentation-build");
const outputDir = path.join(workspaceDir, "Presentation");
const finalPath = path.join(outputDir, "RGB_Enemy_AI_System.pptx");
const skillDir = "C:/Users/manun/.codex/plugins/cache/openai-primary-runtime/presentations/26.1015.12040/skills/presentations";
const pythonExecutable = "C:/Users/manun/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe";
process.env.RUNTIME_NODE_MODULES = "C:/Users/manun/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules";

const W = 1280;
const H = 720;
const BG = "#09111F";
const PANEL = "#111C2E";
const PANEL2 = "#16243A";
const WHITE = "#F5F7FB";
const MUTED = "#AAB7CA";
const RED = "#F04E5E";
const GREEN = "#45D483";
const BLUE = "#4BA7FF";
const YELLOW = "#FFC857";
const LINE = "#2D405C";
const FONT = "Aptos";

function addShape(slide, geometry, x, y, w, h, fill, line = "none", radius) {
  return slide.shapes.add({
    geometry,
    position: { left: x, top: y, width: w, height: h },
    fill,
    line: { style: "solid", fill: line, width: line === "none" ? 0 : 1.5 },
    ...(radius ? { borderRadius: radius } : {}),
  });
}

function addText(slide, text, x, y, w, h, opts = {}) {
  const box = addShape(slide, "textbox", x, y, w, h, "none");
  box.text = text;
  box.text.style = {
    typeface: opts.font ?? FONT,
    fontSize: opts.size ?? 24,
    bold: opts.bold ?? false,
    color: opts.color ?? WHITE,
    horizontalAlignment: opts.align ?? "left",
    verticalAlignment: opts.valign ?? "top",
    autoFit: "shrinkText",
  };
  return box;
}

function baseSlide(presentation, title, section, number) {
  const slide = presentation.slides.add();
  slide.background.fill = BG;
  addShape(slide, "rect", 0, 0, 16, H, [RED, GREEN, BLUE][(number - 1) % 3]);
  addText(slide, section.toUpperCase(), 64, 34, 500, 24, { size: 13, bold: true, color: BLUE });
  addText(slide, title, 64, 66, 1130, 62, { size: 34, bold: true });
  addShape(slide, "rect", 64, 137, 1152, 2, LINE);
  addText(slide, String(number).padStart(2, "0"), 1160, 667, 56, 24, { size: 13, bold: true, color: MUTED, align: "right" });
  return slide;
}

function bulletList(slide, items, x, y, w, opts = {}) {
  const size = opts.size ?? 23;
  const gap = opts.gap ?? 70;
  items.forEach((item, i) => {
    const yy = y + i * gap;
    addShape(slide, "ellipse", x, yy + 9, 12, 12, item.color ?? BLUE);
    addText(slide, item.text ?? item, x + 28, yy, w - 28, gap - 6, { size, color: item.textColor ?? WHITE });
  });
}

function labelNode(slide, text, x, y, w, h, color, subtext = "") {
  addShape(slide, "roundRect", x, y, w, h, PANEL2, color, "rounded-xl");
  addShape(slide, "rect", x, y, 8, h, color);
  addText(slide, text, x + 24, y + 18, w - 42, 32, { size: 21, bold: true });
  if (subtext) addText(slide, subtext, x + 24, y + 52, w - 42, h - 64, { size: 15, color: MUTED });
}

function line(slide, x, y, w, h = 3, color = LINE) {
  addShape(slide, "rect", x, y, w, h, color);
}

function addNotes(slide, text, sections) {
  slide.speakerNotes.text = `${text}\n\nSource: ENEMY_AI_SYSTEM_EXPLAINED.txt, ${sections}.`;
  slide.speakerNotes.setVisible(true);
}

async function writeBlob(filePath, blob) {
  await fs.writeFile(filePath, new Uint8Array(await blob.arrayBuffer()));
}

async function main() {
  await fs.mkdir(buildDir, { recursive: true });
  await fs.mkdir(outputDir, { recursive: true });
  const p = Presentation.create({ slideSize: { width: W, height: H } });

  // 1 — Cover
  {
    const s = p.slides.add();
    s.background.fill = BG;
    addShape(s, "rect", 0, 0, 18, H, RED);
    addShape(s, "rect", 18, 0, 18, H, GREEN);
    addShape(s, "rect", 36, 0, 18, H, BLUE);
    addText(s, "RGB", 100, 100, 320, 100, { size: 72, bold: true });
    addText(s, "Enemy AI System", 100, 205, 760, 80, { size: 48, bold: true });
    addText(s, "Coordinated enemy behavior in an Unreal Engine 5.7 action-puzzle platformer", 104, 310, 700, 92, { size: 24, color: MUTED });
    labelNode(s, "COORDINATE", 880, 150, 270, 90, RED, "Choose who attacks");
    labelNode(s, "SPECIALIZE", 930, 280, 270, 90, GREEN, "Move in distinct ways");
    labelNode(s, "REVALIDATE", 850, 410, 270, 90, BLUE, "Check before impact");
    addText(s, "AI SYSTEM PRESENTATION", 104, 640, 400, 24, { size: 14, bold: true, color: BLUE });
    addNotes(s, "Introduce the AI as one part of RGB's main gameplay loop. The system coordinates several enemies while allowing each enemy type to retain its own movement and attack behavior. The presentation focuses on the design problem, the architecture, the three enemy variants, and the technical choices behind the implementation.", "document summary and section 14");
  }

  // 2 — Problem
  {
    const s = baseSlide(p, "The AI design problem", "Why coordination matters", 2);
    addText(s, "Several enemies should create pressure without creating chaos", 64, 165, 735, 58, { size: 29, bold: true });
    bulletList(s, [
      "Enemies need distinct movement and attack identities",
      "Only a limited number should attack simultaneously",
      "Telegraphs must give the player time to react",
      "Designers need behavior they can tune and debug",
    ], 80, 255, 690, { size: 23, gap: 78 });
    addShape(s, "ellipse", 920, 292, 126, 126, "#253752", BLUE);
    addText(s, "PLAYER", 929, 339, 108, 26, { size: 18, bold: true, align: "center" });
    [[815,210,RED,"ATTACK"],[1082,220,GREEN,"PRESSURE"],[1090,460,BLUE,"FLANK"],[790,470,YELLOW,"WAIT"]].forEach(([x,y,c,t])=>{
      addShape(s, "ellipse", x, y, 96, 96, PANEL2, c);
      addText(s, t, x+8, y+35, 80, 22, { size: 13, bold: true, align: "center", color: c });
    });
    addNotes(s, "Begin with the player experience. If every enemy independently runs straight at the player and attacks as soon as possible, encounters become crowded and difficult to read. RGB therefore needs both variety and control: enemies should feel active, but their attacks should remain understandable and fair.", "overview and section 4");
  }

  // 3 — Architecture
  {
    const s = baseSlide(p, "Two-level AI architecture", "System structure", 3);
    labelNode(s, "Arena coordinator", 420, 170, 440, 105, BLUE, "Group roles, attack permission, reservations");
    line(s, 638, 275, 4, 67, BLUE);
    line(s, 235, 340, 808, 4, BLUE);
    [
      [105, 390, "Ground enemy", RED, "Patrol, chase, melee"],
      [470, 390, "Teleporter", GREEN, "Point choice, teleport, AoE"],
      [835, 390, "Flying enemy", BLUE, "Steering, avoidance, pursuit"],
    ].forEach(([x,y,t,c,st]) => {
      line(s, x+170, 342, 4, 48, c);
      labelNode(s, t, x, y, 340, 118, c, st);
    });
    addText(s, "Arena control owns activation, respawn, reset, and completion", 280, 560, 720, 44, { size: 22, bold: true, align: "center" });
    addText(s, "C++ owns gameplay rules. Blueprints provide assets and tuning.", 280, 615, 720, 32, { size: 18, color: MUTED, align: "center" });
    addNotes(s, "The coordinator makes decisions that require shared knowledge, such as choosing attackers or reserving a teleport destination. Each enemy actor then executes its own movement and attack. Arena control sits around both layers and handles lifecycle events. This structure avoids one enormous AI class while still giving the group consistent rules.", "sections 1 and 4");
  }

  // 4 — Shared state
  {
    const s = baseSlide(p, "Shared state and coordination roles", "Common vocabulary", 4);
    addText(s, "Action states make behavior visible and debuggable", 64, 166, 620, 40, { size: 27, bold: true });
    const states = [["IDLE",MUTED],["PATROL",GREEN],["CHASE",BLUE],["WINDUP",YELLOW],["COOLDOWN",RED],["DISABLED","#7E8AA0"]];
    states.forEach(([t,c],i)=>{
      const x=70+i*194;
      addShape(s,"roundRect",x,245,160,62,PANEL2,c,"rounded-xl");
      addText(s,t,x+12,265,136,22,{size:15,bold:true,align:"center",color:c});
      if(i<states.length-1) line(s,x+160,275,34,3,LINE);
    });
    addText(s, "Roles describe each enemy’s current job", 64, 365, 620, 40, { size: 27, bold: true });
    labelNode(s, "ATTACKER", 80, 435, 330, 105, RED, "May request attack permission");
    labelNode(s, "MELEE PRESSURE", 475, 435, 330, 105, GREEN, "Occupies a side of the player");
    labelNode(s, "RANGED PRESSURE", 870, 435, 330, 105, BLUE, "Maintains broader arena pressure");
    addText(s, "Leases expire, and commitment time reduces rapid role swapping", 230, 590, 820, 40, { size: 20, color: MUTED, align: "center" });
    addNotes(s, "The shared action-state enum gives all enemy variants the same language for debugging. Coordination roles answer a different question: what job should this enemy perform for the group? Assignments have a lease, so an enemy cannot follow stale instructions forever. Commitment time also prevents two similarly positioned enemies from repeatedly swapping roles.", "sections 2, 3, and 4");
  }

  // 5 — Selection
  {
    const s = baseSlide(p, "Attack selection and shared resources", "Multi-agent coordination", 5);
    const steps = [
      ["1", "Collect eligible enemies", BLUE],
      ["2", "Sort by distance and commitment", GREEN],
      ["3", "Choose a limited attacker set", RED],
      ["4", "Assign pressure positions", YELLOW],
    ];
    steps.forEach(([n,t,c],i)=>{
      const y=176+i*104;
      addShape(s,"ellipse",76,y,54,54,c);
      addText(s,n,76,y+12,54,28,{size:20,bold:true,align:"center",color:BG});
      addText(s,t,158,y+8,480,40,{size:24,bold:true});
      if(i<steps.length-1) line(s,102,y+54,3,50,LINE);
    });
    addShape(s,"roundRect",720,184,450,368,PANEL,LINE,"rounded-2xl");
    addText(s,"SHARED SAFETY RULES",760,220,370,28,{size:15,bold:true,color:BLUE,align:"center"});
    labelNode(s,"Attack permission",770,280,350,90,RED,"Caps simultaneous attacks");
    labelNode(s,"Target reservation",770,400,350,90,GREEN,"Stops teleport destination conflicts");
    addText(s,"The group coordinates scarce opportunities while each actor executes locally",690,590,500,52,{size:20,bold:true,align:"center"});
    addNotes(s, "The coordinator updates on a timer. It removes invalid references, finds the player, gathers candidates, sorts them, and preserves an existing committed attacker when appropriate. It then selects up to the configured maximum. Attack permission and teleport reservations operate like shared resources that enemies must acquire before continuing.", "section 4");
  }

  // 6 — Ground
  {
    const s = baseSlide(p, "Ground enemy behavior", "Enemy variant 1", 6);
    addText(s,"Local movement plus coordinated intent",64,164,600,42,{size:28,bold:true});
    const phases=[["PATROL","Wall and ledge probes",GREEN],["DETECT","Range with hysteresis",BLUE],["CHASE","Move toward assignment",YELLOW],["ATTACK","Windup, hit check, cooldown",RED]];
    phases.forEach(([a,b,c],i)=>{
      const x=68+i*295;
      addShape(s,"roundRect",x,260,250,150,PANEL2,c,"rounded-xl");
      addText(s,a,x+20,284,210,28,{size:18,bold:true,color:c,align:"center"});
      addText(s,b,x+25,330,200,55,{size:18,align:"center",color:WHITE});
      if(i<phases.length-1) line(s,x+250,332,45,4,LINE);
    });
    addShape(s,"roundRect",210,480,860,94,"#18263C",LINE,"rounded-xl");
    addText(s,"Damage happens only after the windup and a fresh range check",240,503,800,32,{size:24,bold:true,align:"center"});
    addText(s,"This keeps the attack readable and prevents stale hits",260,543,760,26,{size:17,color:MUTED,align:"center"});
    addNotes(s, "The ground enemy patrols only when no coordinated movement takes priority. A forward sphere sweep detects blocking geometry, while a downward trace checks for safe ground ahead. Detection uses a larger lose-interest range to prevent rapid target flickering. During an attack, a delayed timer performs a fresh range check before applying damage.", "sections 5 and 6");
  }

  // 7 — Teleporter
  {
    const s = baseSlide(p, "Teleporter enemy behavior", "Enemy variant 2", 7);
    addText(s,"Authored destinations create controlled unpredictability",64,164,760,42,{size:28,bold:true});
    addShape(s,"ellipse",885,210,110,110,PANEL2,GREEN);
    addText(s,"START",897,251,86,24,{size:16,bold:true,align:"center",color:GREEN});
    const pts=[[770,410,"REJECTED",RED],[970,390,"RESERVED",YELLOW],[1090,520,"VALID",GREEN],[760,550,"VALID",GREEN]];
    pts.forEach(([x,y,t,c])=>{
      addShape(s,"ellipse",x,y,80,80,PANEL2,c);
      addText(s,t,x+5,y+29,70,18,{size:11,bold:true,align:"center",color:c});
    });
    bulletList(s,[
      "Discover tagged Target Points once at startup",
      "Reject occupied, repeated, reserved, or nearby points",
      "Blend player proximity with a random score",
      "Reserve during the visible charge and validate again",
      "Use a warned area attack when the player remains close",
    ],76,240,630,{size:21,gap:72});
    addText(s,"Closer points are favored, but the result remains unpredictable",720,625,490,30,{size:17,color:MUTED,align:"center"});
    addNotes(s, "Level designers place and tag the legal teleport points. The enemy filters unsafe choices, scores the remaining points using randomness and player proximity, and reserves the winner before charging. At the end of the charge, it validates the capsule location again. Its area attack follows the same fairness rule: warn first, then recheck the player before damage.", "section 8");
  }

  // 8 — Flyer
  {
    const s = baseSlide(p, "Flying enemy behavior", "Enemy variant 3", 8);
    addText(s,"Steering combines several local influences",64,164,680,42,{size:28,bold:true});
    labelNode(s,"PLAYER PURSUIT",80,245,300,100,BLUE,"Move toward the live player position");
    labelNode(s,"SEPARATION",80,385,300,100,GREEN,"Push away from nearby enemies");
    labelNode(s,"OBSTACLE AVOIDANCE",80,525,300,100,RED,"Prefer directions with clearance");
    line(s,400,293,100,4,BLUE); line(s,400,433,100,4,GREEN); line(s,400,573,100,4,RED);
    addShape(s,"ellipse",520,357,190,190,PANEL2,BLUE);
    addText(s,"DESIRED\nVELOCITY",540,415,150,70,{size:22,bold:true,align:"center"});
    line(s,710,449,95,4,YELLOW);
    labelNode(s,"SAFE MOVEMENT",825,390,345,120,YELLOW,"Acceleration, swept collision, sliding, camera-zone clamp");
    addText(s,"Local steering handles nearby geometry without a full path planner",690,575,500,48,{size:20,bold:true,align:"center"});
    addNotes(s, "The flyer calculates a desired velocity from pursuit, separation, and obstacle avoidance. It approaches that velocity gradually, which produces smoother motion. Movement uses collision sweeps and a second slide movement after a hit. Before moving, the target position is clamped to the camera zone so the enemy stays within the active arena.", "section 7");
  }

  // 9 — Integration
  {
    const s = baseSlide(p, "Combat, color, and arena lifecycle", "Gameplay integration", 9);
    const flow=[
      ["MATCHING\nPROJECTILE",BLUE],
      ["HEALTH\nREDUCED",RED],
      ["ENEMY\nELIMINATED",YELLOW],
      ["PLATFORM COLOR\nTRANSFER",GREEN],
      ["ARENA\nRESPAWN",BLUE],
    ];
    flow.forEach(([t,c],i)=>{
      const x=55+i*242;
      addShape(s,"roundRect",x,235,195,105,PANEL2,c,"rounded-xl");
      addText(s,t,x+18,266,159,48,{size:17,bold:true,align:"center",color:c});
      if(i<flow.length-1) line(s,x+195,286,47,4,LINE);
    });
    addText(s,"Grounded enemies can transfer their color to the platform beneath them",160,410,960,46,{size:25,bold:true,align:"center"});
    addShape(s,"roundRect",185,495,910,105,PANEL,LINE,"rounded-xl");
    addText(s,"Arena control records spawn configuration, removes eliminated enemies from coordination, and restores configured instances after the delay",225,520,830,62,{size:20,align:"center"});
    addNotes(s, "Enemy AI is tied directly to the color puzzle. A hit only succeeds when the projectile color matches the enemy. On elimination, a grounded enemy can transfer its color to its current RGB platform. Arena control then handles event cleanup and optionally restores the same configured enemy instance after a fixed or progress-scaled delay.", "sections 9 and 10");
  }

  // 10 — Decision
  {
    const s = baseSlide(p, "Why deterministic C++ AI was chosen", "Technical decision", 10);
    addText(s,"Unreal-MAP was investigated as a reinforcement-learning option",64,164,900,42,{size:27,bold:true});
    labelNode(s,"Potential value",70,245,500,115,GREEN,"Automated simulations, learned cooperation, strategy comparison");
    labelNode(s,"Integration cost",70,410,500,145,RED,"Python training pipeline, new environment interfaces, extra dependencies, UE 4.27 to 5.7 migration work");
    addShape(s,"roundRect",685,235,500,330,"#14243A",BLUE,"rounded-2xl");
    addText(s,"DECISION",725,275,420,28,{size:15,bold:true,color:BLUE,align:"center"});
    addText(s,"Continue with the established state-based C++ architecture",735,338,400,110,{size:31,bold:true,align:"center"});
    addText(s,"Directly tunable in Unreal\nEasier to debug\nAchievable within project scope",760,478,350,70,{size:18,color:MUTED,align:"center"});
    addText(s,"Reinforcement learning remains a possible future research direction",675,602,520,32,{size:18,color:MUTED,align:"center"});
    addNotes(s, "Unreal-MAP could support training and evaluation across many simulated encounters. However, it is a broader research environment rather than a small AI plugin. Integrating it would require communication interfaces, observations, actions, rewards, episode reset logic, and engine-version migration. Since RGB already had working C++ AI, deterministic coordination offered the best balance of quality and scope.", "section 15");
  }

  // 11 — Closing
  {
    const s = baseSlide(p, "Strengths, limitations, and next steps", "Conclusion", 11);
    addText(s,"CURRENT FOUNDATION",74,180,320,26,{size:15,bold:true,color:GREEN});
    bulletList(s,[
      "Explicit states and shared permissions",
      "Distinct movement for three enemy types",
      "Timer-driven decisions and repeated validation",
      "C++ rules with Blueprint tuning",
    ],78,230,500,{size:21,gap:70});
    addText(s,"FUTURE DEVELOPMENT",700,180,360,26,{size:15,bold:true,color:YELLOW});
    bulletList(s,[
      "Traversal between separated platforms",
      "Puzzle-aware sabotage decisions",
      "Cannon-aware tactics",
      "Advanced path planning and networking",
    ],704,230,500,{size:21,gap:70});
    addShape(s,"roundRect",180,545,920,82,"#182A42",BLUE,"rounded-xl");
    addText(s,"The current system is a stable, debuggable base for more advanced enemy decision-making",220,568,840,34,{size:24,bold:true,align:"center"});
    addNotes(s, "Close by separating completed work from future ideas. The current implementation already coordinates attacks, handles three specialized enemy types, integrates with color combat, and manages arena lifecycle. It does not yet provide global navigation, puzzle-aware sabotage, cannon tactics, or networking. Those limitations define a clear direction for future work rather than weakening the current system.", "sections 12, 13, and 16");
  }

  for (let i = 0; i < p.slides.items.length; i++) {
    const slide = p.slides.items[i];
    await writeBlob(path.join(buildDir, `slide-${String(i+1).padStart(2,"0")}.png`), await p.export({ slide, format: "png", scale: 1 }));
  }
  await writeBlob(path.join(buildDir, "montage.webp"), await p.export({ format: "webp", montage: true }));

  const { finalizePresentation } = await import(pathToFileURL(path.join(skillDir, "container_tools/artifact_tool_utils.mjs")).href);
  const stagingDir = path.join(buildDir, "finalizer");
  await fs.mkdir(stagingDir, { recursive: true });
  const candidatePath = path.join(stagingDir, "candidate.pptx");
  await (await PresentationFile.exportPptx(p)).save(candidatePath);
  const result = await finalizePresentation({
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
    receiptPath: path.join(stagingDir, "RGB_Enemy_AI_System.validation.json"),
  });
  console.log(JSON.stringify(result, null, 2));
  console.log(finalPath);
}

main().catch((e) => { console.error(e); process.exitCode = 1; });
