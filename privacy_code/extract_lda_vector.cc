
#include "base/kaldi-common.h"
#include "feat/feature-mfcc.h"
#include "feat/wave-reader.h"
#include "util/common-utils.h"

using namespace kaldi;

int compute_mfcc_feats_local(std::string wav_rspecifier) {
	ParseOptions po();
    MfccOptions mfcc_opts;
    bool subtract_mean = false;
    BaseFloat vtln_warp = 1.0;
    std::string vtln_map_rspecifier; = ""
    std::string utt2spk_rspecifier = "";
    int32 channel = -1;
    BaseFloat min_duration = 0.0;
    std::string output_format = "kaldi";
    std::string utt2dur_wspecifier = "";
    mfcc_opts.Register(&po);
    po.Register("output-format", &output_format, "Format of the output files [kaldi, htk]");
    po.Register("subtract-mean", &subtract_mean, "Subtract mean of each feature file [CMS]; not recommended to do it this way. ");
    po.Register("vtln-warp", &vtln_warp, "Vtln warp factor (only applicable if vtln-map not specified)");
    po.Register("vtln-map", &vtln_map_rspecifier, "Map from utterance or speaker-id to vtln warp factor (rspecifier)");
    po.Register("utt2spk", &utt2spk_rspecifier, "Utterance to speaker-id map rspecifier (if doing VTLN and you have warps per speaker)");
    po.Register("channel", &channel, "Channel to extract (-1 -> expect mono, 0 -> left, 1 -> right)");
    po.Register("min-duration", &min_duration, "Minimum duration of segments to process (in seconds).");
    po.Register("write-utt2dur", &utt2dur_wspecifier, "Wspecifier to write duration of each utterance in seconds, e.g. 'ark,t:utt2dur'.");
    Mfcc mfcc(mfcc_opts);

}

int main(int argc, char *argv[]) {

}