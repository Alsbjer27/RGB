import fs from "node:fs/promises";
import { FileBlob, PresentationFile } from "file:///C:/Users/manun/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/@oai/artifact-tool/dist/artifact_tool.mjs";

const input = "C:/Users/manun/Desktop/Spelprojekt/RGB/Presentation/RGB_Enemy_AI_System.pptx";
const out = "C:/Users/manun/Desktop/Spelprojekt/RGB/.codex-presentation-build/existing-review";
await fs.mkdir(out, { recursive: true });
const p = await PresentationFile.importPptx(await FileBlob.load(input));
const inspected = await p.inspect({
  select: "$.slides[*]",
  include: ["id", "index", "title", "elements.id", "elements.kind", "elements.text", "notes.text"],
  fileName: `${out}/content.ndjson`,
});
await fs.writeFile(`${out}/content-copy.ndjson`, inspected.ndjson);
for (let i = 0; i < p.slides.items.length; i++) {
  const blob = await p.export({ slide: p.slides.items[i], format: "png", scale: 1 });
  await fs.writeFile(`${out}/slide-${String(i + 1).padStart(2, "0")}.png`, new Uint8Array(await blob.arrayBuffer()));
}
const montage = await p.export({ format: "webp", montage: true });
await fs.writeFile(`${out}/montage.webp`, new Uint8Array(await montage.arrayBuffer()));
console.log(`slides=${p.slides.items.length}`);
