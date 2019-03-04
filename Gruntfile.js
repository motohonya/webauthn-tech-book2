"use strict";

let fs = require("fs");
let yaml = require("js-yaml");

const articles = "articles";
const bookConfig = yaml.safeLoad(fs.readFileSync(`${articles}/config.yml`, "utf8"));

const reviewPrefix = process.env["REVIEW_PREFIX"] || "bundle exec ";
const reviewPostfix = process.env["REVIEW_POSTFIX"] || "";             // REVIEW_POSTFIX="-peg" npm run pdf とかするとPEGでビルドできるよ
const reviewPreproc = `${reviewPrefix}review-preproc${reviewPostfix}`;
const reviewCompile = `${reviewPrefix}review-compile${reviewPostfix}`;
const reviewPdfMaker = `${reviewPrefix}review-pdfmaker${reviewPostfix}`;
const reviewEpubMaker = `${reviewPrefix}review-epubmaker${reviewPostfix}`;

module.exports = grunt => {
	grunt.initConfig({
		clean: {
			review: {
				src: [
					`${articles}/${bookConfig.bookname}-*/`, // pdf, epub temp dir
					`${articles}/*.pdf`,
					`${articles}/*.epub`,
					`${articles}/*.html`,
					`${articles}/*.md`,
					`${articles}/*.xml`,
					`${articles}/*.txt`
				]
			}
		},
		shell: {
			preprocess: {
				options: {
					execOptions: {
						cwd: articles,
					}
				},
				command: `${reviewPreproc} -r --tabwidth=2 *.re`
			},
			compile2text: {
				options: {
					execOptions: {
						cwd: articles,
					}
				},
				command: `${reviewCompile} --target=text`
			},
			compile2markdown: {
				options: {
					execOptions: {
						cwd: articles,
					}
				},
				command: `${reviewCompile} --target=markdown`
			},
			compile2html: {
				options: {
					execOptions: {
						cwd: articles,
					}
				},
				command: `${reviewCompile} --target=html --stylesheet=style.css --chapterlink`
			},
			compile2latex: {
				options: {
					execOptions: {
						cwd: articles,
					}
				},
				command: `${reviewCompile} --target=latex --footnotetext`
			},
			compile2idgxml: {
				options: {
					execOptions: {
						cwd: articles,
					}
				},
				command: `${reviewCompile} --target=idgxml`
			},
			compile2pdf: {
				options: {
					execOptions: {
						cwd: articles,
					}
				},
				command: `${reviewPdfMaker} config.yml`
			},
			compile2epub: {
				options: {
					execOptions: {
						cwd: articles,
					}
				},
				command: `${reviewEpubMaker} config.yml`
			}
		}
	});

	function generateTask(target) {
		return ["clean", "shell:preprocess", `shell:compile2${target}`];
	}

	grunt.registerTask(
		"default",
		"原稿をコンパイルしてPDFファイルにする",
		"pdf");

	grunt.registerTask(
		"text",
		"原稿をコンパイルしてTextファイルにする",
		generateTask("text"));

	grunt.registerTask(
		"markdown",
		"原稿をコンパイルしてMarkdownファイルにする",
		generateTask("markdown"));

	grunt.registerTask(
		"html",
		"原稿をコンパイルしてHTMLファイルにする",
		generateTask("html"));

	grunt.registerTask(
		"idgxml",
		"原稿をコンパイルしてInDesign用XMLファイルにする",
		generateTask("idgxml"));

	grunt.registerTask(
		"pdf",
		"原稿をコンパイルしてpdfファイルにする",
		generateTask("pdf"));

	grunt.registerTask(
		"epub",
		"原稿をコンパイルしてepubファイルにする",
		generateTask("epub"));

	require('load-grunt-tasks')(grunt);
};
