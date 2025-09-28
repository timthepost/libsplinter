import lumeCMS from "lume/cms/mod.ts";

const cms = lumeCMS({
  site: {
    name: "Your Site",
    description: "Your Description",
  },
});

/**
 * Very sparse initial support - fields need setup before this
 * is useful.
 */
cms.collection("blogs", "src:blog/*.mdx", [
  "title: text",
  "content: markdown",
]);

cms.collection("docs", "src:docs/*.mdx", [
  "title: text",
  "content: markdown",
]);

cms.collection("pages", "src:pages/*.mdx", [
  "title: text",
  "content: markdown",
]);

export default cms;
